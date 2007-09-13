/*
 * Copyright 2004,2005 The Apache Software Foundation.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
 
#include "savan_db_mgr.h"
#include <axutil_log.h>
#include <axutil_hash.h>
#include <axutil_thread.h>
#include <axutil_property.h>
#include <axis2_conf_ctx.h>
#include <axutil_types.h>
#include <platforms/axutil_platform_auto_sense.h>
#include <savan_constants.h>
#include <savan_util.h>

/**
 * Savan subscription manager maintain two database tables in mysql. They are namely
 * subscriber and topic.
 * subscriber table has following schema
 *  id varchar(100) primary key, 
    end_to varchar(200), 
    notify_to varchar(200), 
    delivery_mode varchar(100), 
    expires varchar(100), 
    filter varchar(200), 
    topic_name varchar(100), 
    renewed boolean
 *
 * topic table has following schema
 * name varchar(100) primary key,
   url varchar(200)
 *
 */

int
savan_db_mgr_busy_handler(
    sqlite3* dbconn,
    char *sql_stmt,
    int (*callback_func)(void *, int, char **, char **),
    void *args,
    char **error_msg,
    int rc);


AXIS2_EXTERN savan_db_mgr_t * AXIS2_CALL
savan_db_mgr_create(
    const axutil_env_t *env,
    axis2_conf_ctx_t *conf_ctx)
{
    savan_db_mgr_t *db_mgr = NULL;
    
    AXIS2_ENV_CHECK(env, NULL);
    db_mgr = AXIS2_MALLOC(env->allocator, sizeof(savan_db_mgr_t));
    db_mgr->conf_ctx = conf_ctx;

    return db_mgr;
}

void AXIS2_CALL
savan_db_mgr_free(
    savan_db_mgr_t *db_mgr,
    const axutil_env_t *env)
{
    AXIS2_LOG_DEBUG(env->log, AXIS2_LOG_SI,
        "[SAVAN] Start:savan_db_mgr_free");
    if(db_mgr)
    {
        AXIS2_FREE(env->allocator, db_mgr);
    }

    AXIS2_LOG_DEBUG(env->log, AXIS2_LOG_SI,
        "[SAVAN] Exit:savan_db_mgr_free");
}

int AXIS2_CALL
savan_db_mgr_topic_find_callback(
    void *not_used, 
    int argc, 
    char **argv, 
    char **col_name)
{
    int i = 0;
    savan_db_mgr_args_t *args = (savan_db_mgr_args_t *) not_used;
    const axutil_env_t *env = args->env;
    axutil_array_list_t *topic_list = (axutil_array_list_t *) args->data;
    if(argc < 1)
    {
        args->data = NULL;
        return 0;
    }
    if(!topic_list)
    {
        topic_list = axutil_array_list_create(env, 0);
        args->data = topic_list;
    }
    for(i = 0; i < argc; i++)
    {
        if(0 == axutil_strcmp(col_name[i], "topic_url"))
        {
            axutil_array_list_add(topic_list, env, argv[i]);
        }
    }
    return 0;
}
int
savan_db_mgr_subs_find_callback(
    void *not_used, 
    int argc, 
    char **argv, 
    char **col_name)
{
    savan_subscriber_t *subscriber = NULL;
    int i = 0;
    savan_db_mgr_args_t *args = (savan_db_mgr_args_t *) not_used;
    const axutil_env_t *env = args->env;
    axutil_array_list_t *subscriber_list = (axutil_array_list_t *) args->data;
    if(argc < 1)
    {
        args->data = NULL;
        return 0;
    }
    if(!subscriber_list)
    {
        subscriber_list = axutil_array_list_create(env, 0);
        args->data = subscriber_list;
    }
    if(argc > 0)
    {
        subscriber = savan_subscriber_create(env);
    }
    for(i = 0; i < argc; i++)
    {
        if(0 == axutil_strcmp(col_name[i], "id"))
        {
            savan_subscriber_set_id(subscriber, env, argv[i]);
        }
        if(0 == axutil_strcmp(col_name[i], "end_to"))
        {
            axis2_endpoint_ref_t *endto_epr = NULL;
            endto_epr = axis2_endpoint_ref_create(env, argv[i]);
            savan_subscriber_set_end_to(subscriber, env, endto_epr);
        }
        if(0 == axutil_strcmp(col_name[i], "notify_to"))
        {
            axis2_endpoint_ref_t *notify_epr = NULL;
            notify_epr = axis2_endpoint_ref_create(env, argv[i]);
            savan_subscriber_set_notify_to(subscriber, env, notify_epr);
        }
        if(0 == axutil_strcmp(col_name[i], "delivery_mode"))
        {
            savan_subscriber_set_delivery_mode(subscriber, env, argv[i]);
        }
        if(0 == axutil_strcmp(col_name[i], "expires"))
        {
            savan_subscriber_set_expires(subscriber, env, argv[i]);
        }
        if(0 == axutil_strcmp(col_name[i], "filter"))
        {
            savan_subscriber_set_filter(subscriber, env, argv[i]);
        }
        if(0 == axutil_strcmp(col_name[i], "topic_url"))
        {
            savan_subscriber_set_topic(subscriber, env, argv[i]);
        }
        if(0 == axutil_strcmp(col_name[i], "renewed"))
        {
            savan_subscriber_set_renew_status(subscriber, env, 
                AXIS2_ATOI(argv[i]));
        }
    }
    if(subscriber)
        axutil_array_list_add(subscriber_list, env, subscriber);
    return 0;
}

int  
savan_db_mgr_subs_retrieve_callback(
    void *not_used, 
    int argc, 
    char **argv, 
    char **col_name)
{
    int i = 0;
	savan_subscriber_t *subscriber = NULL;
	const axutil_env_t *env = NULL;
	savan_db_mgr_args_t *args = (savan_db_mgr_args_t *) not_used;
    env = args->env;

	AXIS2_LOG_DEBUG(env->log, AXIS2_LOG_SI, 
        "[SAVAN] Start:savan_db_mgr_subs_retrieve_callback");
     subscriber = (savan_subscriber_t *) args->data;
    if(argc < 1)
    {
        args->data = NULL;
        return 0;
    }
    if(!subscriber && argc > 0)
    {
        subscriber = savan_subscriber_create(env);
        args->data = subscriber;
    }
    for(i = 0; i < argc; i++)
    {
        if(0 == axutil_strcmp(col_name[i], "id"))
        {
            savan_subscriber_set_id(subscriber, env, argv[i]);
        }
        if(0 == axutil_strcmp(col_name[i], "end_to"))
        {
            axis2_endpoint_ref_t *endto_epr = NULL;
            endto_epr = axis2_endpoint_ref_create(env, argv[i]);
            savan_subscriber_set_end_to(subscriber, env, endto_epr);
        }
        if(0 == axutil_strcmp(col_name[i], "notify_to"))
        {
            axis2_endpoint_ref_t *notify_epr = NULL;
            notify_epr = axis2_endpoint_ref_create(env, argv[i]);
            savan_subscriber_set_notify_to(subscriber, env, notify_epr);
        }
        if(0 == axutil_strcmp(col_name[i], "delivery_mode"))
        {
            savan_subscriber_set_delivery_mode(subscriber, env, argv[i]);
        }
        if(0 == axutil_strcmp(col_name[i], "expires"))
        {
            savan_subscriber_set_expires(subscriber, env, argv[i]);
        }
        if(0 == axutil_strcmp(col_name[i], "filter"))
        {
            savan_subscriber_set_filter(subscriber, env, argv[i]);
        }
        if(0 == axutil_strcmp(col_name[i], "topic_url"))
        {
            savan_subscriber_set_topic(subscriber, env, argv[i]);
        }
        if(0 == axutil_strcmp(col_name[i], "renewed"))
        {
            savan_subscriber_set_renew_status(subscriber, env, 
                AXIS2_ATOI(argv[i]));
        }
    }
    AXIS2_LOG_DEBUG(env->log, AXIS2_LOG_SI, 
        "[SAVAN] End:savan_db_mgr_subs_retrieve_callback");
    return 0;
}

axis2_status_t AXIS2_CALL
savan_db_mgr_insert(
    savan_db_mgr_t *db_mgr,
    const axutil_env_t *env,
    axis2_char_t *sql_stmt_insert)
{
    char *error_msg;
    int rc = -1;
    sqlite3 *dbconn = NULL;
    AXIS2_ENV_CHECK(env, AXIS2_FALSE);
   
    dbconn = (sqlite3 *) savan_db_mgr_get_dbconn(
        db_mgr, env);
    if(!dbconn)
        return AXIS2_FALSE;
    rc = sqlite3_exec(dbconn, "BEGIN;", 0, 0, &error_msg);
    if(rc == SQLITE_BUSY)
    {
        rc = savan_db_mgr_busy_handler(dbconn,
            "BEGIN;", 0, 0, &error_msg, rc);
    }
    rc = sqlite3_exec(dbconn, sql_stmt_insert, 0, 0, &error_msg);
    if(rc == SQLITE_BUSY)
        rc = savan_db_mgr_busy_handler(dbconn, sql_stmt_insert, 
            0, 0, &error_msg, rc);
    if( rc != SQLITE_OK )
    {
        rc = sqlite3_exec(dbconn, "ROLLBACK;", 0, 0, &error_msg);
        if(rc == SQLITE_BUSY)
        {
            rc = savan_db_mgr_busy_handler(dbconn,
                "ROLLBACK;", 0, 0, &error_msg, rc);
        }
        AXIS2_LOG_ERROR(env->log, AXIS2_LOG_SI, 
            "[SAVAN] Error Sql Insert Stmt: %s. Sql Insert Error: %s", 
            sql_stmt_insert, error_msg);
        sqlite3_free(error_msg);
        sqlite3_close(dbconn);
        return AXIS2_FAILURE;
    }
    rc = sqlite3_exec(dbconn, "COMMIT;", 0, 0, &error_msg);
    if(rc == SQLITE_BUSY)
    {
        rc = savan_db_mgr_busy_handler(dbconn,
            "COMMIT;", 0, 0, &error_msg, rc);
    }
    sqlite3_close(dbconn);
    return AXIS2_SUCCESS;
}

axis2_status_t AXIS2_CALL
savan_db_mgr_remove(
    savan_db_mgr_t *db_mgr,
    const axutil_env_t *env,
    axis2_char_t *sql_stmt_remove)
{
    axis2_char_t *error_msg = NULL;
    sqlite3 *dbconn = NULL;
    int rc = -1;
    AXIS2_ENV_CHECK(env, AXIS2_FALSE);
    dbconn = (sqlite3 *) savan_db_mgr_get_dbconn(
        db_mgr, env);
    if(!dbconn)
        return AXIS2_FALSE;
    rc = sqlite3_exec(dbconn, "BEGIN;", 0, 0, &error_msg);
    if(rc == SQLITE_BUSY)
    {
        rc = savan_db_mgr_busy_handler(dbconn,
            "BEGIN;", 0, 0, &error_msg, rc);
    }
    rc = sqlite3_exec(dbconn, sql_stmt_remove, 0, 0, &error_msg);
    if(rc == SQLITE_BUSY)
        rc = savan_db_mgr_busy_handler(dbconn, sql_stmt_remove, 
            0, 0, &error_msg, rc);
    if(rc != SQLITE_OK )
    {
        rc = sqlite3_exec(dbconn, "ROLLBACK;", 0, 0, &error_msg);
        if(rc == SQLITE_BUSY)
        {
            rc = savan_db_mgr_busy_handler(dbconn,
                "ROLLBACK;", 0, 0, &error_msg, rc);
        }
        AXIS2_LOG_ERROR(env->log, AXIS2_LOG_SI, 
            "[SAVAN] Error Sql Remove Stmt: %s. Sql Error: %s", sql_stmt_remove, 
                error_msg);
        sqlite3_free(error_msg);
        sqlite3_close(dbconn);
        return AXIS2_FAILURE;
    }
    rc = sqlite3_exec(dbconn, "COMMIT;", 0, 0, &error_msg);
    if(rc == SQLITE_BUSY)
    {
        rc = savan_db_mgr_busy_handler(dbconn,
            "COMMIT;", 0, 0, &error_msg, rc);
    }
    sqlite3_close(dbconn);
    return AXIS2_SUCCESS;
}

axis2_status_t AXIS2_CALL
savan_db_mgr_update(
    savan_db_mgr_t *db_mgr,
    const axutil_env_t *env,
    axis2_char_t *sql_stmt_update)
{
    sqlite3 *dbconn = NULL;
    axis2_char_t *error_msg = NULL;
    int rc = -1;
    AXIS2_ENV_CHECK(env, AXIS2_FALSE);
    AXIS2_LOG_DEBUG(env->log, AXIS2_LOG_SI, 
        "[SAVAN] Start:savan_db_mgr_update");
    dbconn = (sqlite3 *) savan_db_mgr_get_dbconn(
        db_mgr, env);
    if(!dbconn)
        return AXIS2_FALSE;
    rc = sqlite3_exec(dbconn, "BEGIN;", 0, 0, &error_msg);
    if(rc == SQLITE_BUSY)
    {
        rc = savan_db_mgr_busy_handler(dbconn,
            "BEGIN;", 0, 0, &error_msg, rc);
    }
    rc = sqlite3_exec(dbconn, sql_stmt_update, 0, 0, &error_msg);
    if(rc == SQLITE_BUSY)
        rc = savan_db_mgr_busy_handler(dbconn, sql_stmt_update, 
            0, 0, &error_msg, rc);
    if(rc != SQLITE_OK )
    {
        rc = sqlite3_exec(dbconn, "ROLLBACK;", 0, 0, &error_msg);
        if(rc == SQLITE_BUSY)
        {
            rc = savan_db_mgr_busy_handler(dbconn,
                "ROLLBACK;", 0, 0, &error_msg, rc);
        }
        AXIS2_LOG_ERROR(env->log, AXIS2_LOG_SI, 
            "[SAVAN] Error Sql Update Stmt:%s", sql_stmt_update);
        AXIS2_LOG_ERROR(env->log, AXIS2_LOG_SI, "[SAVAN] Sql Update Error:%s", 
            error_msg);
        sqlite3_free(error_msg);
        sqlite3_close(dbconn);
        return AXIS2_FAILURE;
    }
    rc = sqlite3_exec(dbconn, "COMMIT;", 0, 0, &error_msg);
    if(rc == SQLITE_BUSY)
    {
        rc = savan_db_mgr_busy_handler(dbconn,
            "COMMIT;", 0, 0, &error_msg, rc);
    }
    sqlite3_close(dbconn);
    AXIS2_LOG_DEBUG(env->log, AXIS2_LOG_SI, "[SAVAN] End:savan_db_mgr_update");
    return AXIS2_SUCCESS;
}

savan_subscriber_t *AXIS2_CALL
savan_db_mgr_retrieve(
    savan_db_mgr_t *db_mgr,
    const axutil_env_t *env,
    int (*retrieve_func)(void *, int, char **, char **),
    axis2_char_t *sql_stmt_retrieve)
{
    savan_db_mgr_args_t *args = NULL;
    axis2_char_t *error_msg = NULL;
    savan_subscriber_t *subscriber = NULL;
    sqlite3 *dbconn = NULL;
    int rc = -1;
    AXIS2_LOG_DEBUG(env->log, AXIS2_LOG_SI, 
        "[SAVAN] Start:savan_db_mgr_retrieve");
    AXIS2_ENV_CHECK(env, AXIS2_FALSE);
    dbconn = (sqlite3 *) savan_db_mgr_get_dbconn(
        db_mgr, env);
    if(!dbconn)
        return NULL;
    rc = sqlite3_exec(dbconn, "BEGIN READ_ONLY;", 0, 0, &error_msg);
    if(rc == SQLITE_BUSY)
    {
        rc = savan_db_mgr_busy_handler(dbconn,
            "BEGIN READ_ONLY;", 0, 0, &error_msg, rc);
    }
    args = AXIS2_MALLOC(env->allocator, sizeof(savan_db_mgr_args_t));
    args->env = (axutil_env_t*)env;
    args->data = NULL;
    rc = sqlite3_exec(dbconn, sql_stmt_retrieve, retrieve_func, args, 
        &error_msg);
    if(rc == SQLITE_BUSY)
        rc = savan_db_mgr_busy_handler(dbconn, sql_stmt_retrieve, 
            retrieve_func, args, &error_msg, rc);
    if(rc != SQLITE_OK )
    {
        rc = sqlite3_exec(dbconn, "ROLLBACK;", 0, 0, &error_msg);
        if(rc == SQLITE_BUSY)
        {
            rc = savan_db_mgr_busy_handler(dbconn,
                "ROLLBACK;", 0, 0, &error_msg, rc);
        }
        AXIS2_LOG_ERROR(env->log, AXIS2_LOG_SI, 
            "[SAVAN] Error Sql Retrieve Stmt: %s. Sql Error: %s", 
                sql_stmt_retrieve, error_msg);
        sqlite3_free(error_msg);
        sqlite3_close(dbconn);
        return AXIS2_FALSE;
    }
    if(args->data)
        subscriber = (savan_subscriber_t *) args->data;
    if(args)
        AXIS2_FREE(env->allocator, args);
    rc = sqlite3_exec(dbconn, "COMMIT;", 0, 0, &error_msg);
    if(rc == SQLITE_BUSY)
    {
        rc = savan_db_mgr_busy_handler(dbconn,
            "COMMIT;", 0, 0, &error_msg, rc);
    }
    sqlite3_close(dbconn);
    AXIS2_LOG_DEBUG(env->log, AXIS2_LOG_SI, 
        "[SAVAN] End:savan_db_mgr_retrieve");
    return subscriber;
}

axutil_array_list_t * AXIS2_CALL
savan_db_mgr_retrieve_all(
    savan_db_mgr_t *db_mgr,
    const axutil_env_t *env,
    int (*find_func)(void *, int, char **, char **),
    axis2_char_t *sql_stmt_find)
{
    savan_db_mgr_args_t *args = NULL;
    axutil_array_list_t *data_list = NULL;
    int rc = -1;
    sqlite3 *dbconn = NULL;
    axis2_char_t *error_msg = NULL;
    AXIS2_ENV_CHECK(env, AXIS2_FALSE);
    data_list = axutil_array_list_create(env, 0);
    if(!data_list)
    {
        AXIS2_ERROR_SET(env->error, AXIS2_ERROR_NO_MEMORY, AXIS2_FAILURE);
        return NULL;
    }
    args = AXIS2_MALLOC(env->allocator, sizeof(savan_db_mgr_args_t));
    args->env = (axutil_env_t*)env;
    args->data = NULL;
    dbconn = (sqlite3 *) savan_db_mgr_get_dbconn(
        db_mgr, env);
    if(!dbconn)
    {
        return NULL;
    }
    rc = sqlite3_exec(dbconn, "BEGIN READ_ONLY;", 0, 0, &error_msg);
    if(rc == SQLITE_BUSY)
    {
        rc = savan_db_mgr_busy_handler(dbconn,
            "BEGIN READ_ONLY;", 0, 0, &error_msg, rc);
    }
    rc = sqlite3_exec(dbconn, sql_stmt_find, find_func, args, 
        &error_msg);
    if(rc == SQLITE_BUSY)
        rc = savan_db_mgr_busy_handler(dbconn, sql_stmt_find, 
            find_func, args, &error_msg, rc);
    if(args->data)
        data_list = (axutil_array_list_t *) args->data;
    if(rc != SQLITE_OK )
    {
        rc = sqlite3_exec(dbconn, "ROLLBACK;", 0, 0, &error_msg);
        if(rc == SQLITE_BUSY)
        {
            rc = savan_db_mgr_busy_handler(dbconn,
                "ROLLBACK;", 0, 0, &error_msg, rc);
        }
        if(data_list)
            axutil_array_list_free(data_list, env);
        if(args)
            AXIS2_FREE(env->allocator, args);
        AXIS2_LOG_ERROR(env->log, AXIS2_LOG_SI, 
            "[SAVAN] Error Sql Retrieve All Stmt:%s", sql_stmt_find); 
        AXIS2_LOG_ERROR(env->log, AXIS2_LOG_SI, 
            "[SAVAN] Sql Retrieve All Error:%s", error_msg);
        sqlite3_free(error_msg);
        sqlite3_close(dbconn);
        return NULL;
    }
    if(args)
        AXIS2_FREE(env->allocator, args);
    rc = sqlite3_exec(dbconn, "COMMIT;", 0, 0, &error_msg);
    if(rc == SQLITE_BUSY)
    {
        rc = savan_db_mgr_busy_handler(dbconn,
            "COMMIT;", 0, 0, &error_msg, rc);
    }
    sqlite3_close(dbconn);
    return data_list;
}

int
savan_db_mgr_busy_handler(
    sqlite3* dbconn,
    char *sql_stmt,
    int (*callback_func)(void *, int, char **, char **),
    void *args,
    char **error_msg,
    int rc)
{
    int counter = 0;
    printf("in busy handler1\n");
    while(rc == SQLITE_BUSY && counter < 512)
    {
        printf("in busy handler\n");
        if(*error_msg)
             sqlite3_free(*error_msg);
        counter++;
        AXIS2_USLEEP(100000);
        rc = sqlite3_exec(dbconn, sql_stmt, callback_func, args, error_msg);
    }
    printf("in busy handler2\n");
    return rc;
}

void * AXIS2_CALL
savan_db_mgr_get_dbconn(
    savan_db_mgr_t *db_mgr, 
    const axutil_env_t *env)
{
    axis2_conf_ctx_t *conf_ctx = db_mgr->conf_ctx;
    axis2_conf_t *conf = NULL; 
    axis2_char_t *path = NULL;
    int rc = -1;
    sqlite3 *dbconn = NULL;

    conf = axis2_conf_ctx_get_conf(conf_ctx, env);
    {
        axis2_module_desc_t *module_desc = NULL;
        axutil_qname_t *qname = NULL;
        qname = axutil_qname_create(env, "savan", NULL, NULL);
        module_desc = axis2_conf_get_module(conf, env, qname);
        if(module_desc)
        {
            axutil_param_t *db_param = NULL;
            db_param = axis2_module_desc_get_param(module_desc, env, SAVAN_DB);
            if(db_param)
            {
                path = (axis2_char_t *) axutil_param_get_value(db_param, env);
            }
        }
        axutil_qname_free(qname, env);
    }
    rc = sqlite3_open(path, &dbconn);
    if(rc != SQLITE_OK)
    {
        AXIS2_LOG_ERROR(env->log, AXIS2_LOG_SI, "Can't open database: %s"
            " sqlite error: %s\n", path, sqlite3_errmsg(dbconn));
        sqlite3_close(dbconn);
        return NULL;
    }
    return dbconn;
}

axis2_char_t *AXIS2_CALL
savan_db_mgr_create_insert_sql(
    const axutil_env_t *env,
    savan_subscriber_t *subscriber,
    axis2_conf_ctx_t *conf_ctx)
{
    axis2_char_t *sql_insert = NULL;
    savan_db_mgr_t *db_mgr = NULL;
    axis2_char_t *id = NULL;
    axis2_char_t *endto = NULL;
    axis2_char_t *notifyto = NULL;
    axis2_char_t *delivery_mode = NULL;
    axis2_char_t *expires = NULL;
    axis2_char_t *filter = NULL;
    axis2_char_t *topic = NULL;
    axis2_char_t *topic_url = NULL;
    axis2_bool_t renewed = AXIS2_FALSE;
    axis2_endpoint_ref_t *endto_epr = NULL;
    axis2_endpoint_ref_t *notifyto_epr = NULL;
    sql_insert = AXIS2_MALLOC(env->allocator, 1024);

    id = savan_subscriber_get_id(subscriber, env);
    endto_epr = savan_subscriber_get_end_to(subscriber, env);
    endto = (axis2_char_t *) axis2_endpoint_ref_get_address(endto_epr, env);
    notifyto_epr = savan_subscriber_get_notify_to(subscriber, env);
    notifyto = (axis2_char_t *) axis2_endpoint_ref_get_address(notifyto_epr, 
        env);
    delivery_mode = savan_subscriber_get_delivery_mode(subscriber, env);
    expires = savan_subscriber_get_expires(subscriber, env);
    filter = savan_subscriber_get_filter(subscriber, env);
    topic_url = savan_subscriber_get_topic(subscriber, env);
    topic = savan_util_get_topic_name_from_topic_url(env, topic_url);
    renewed = savan_subscriber_get_renew_status(subscriber, env);
    db_mgr = savan_db_mgr_create(env, conf_ctx);
    sprintf(sql_insert, "insert into subscriber(id, end_to, notify_to,"\
        "delivery_mode, expires, filter, topic_name, renewed) values('%s'"\
        ", '%s', '%s', '%s', '%s', '%s', '%s', %d);", id, endto, notifyto, 
        delivery_mode, expires, filter, topic, renewed);
    return sql_insert;
}

axis2_char_t *AXIS2_CALL
savan_db_mgr_create_update_sql(
    const axutil_env_t *env,
    savan_subscriber_t *subscriber,
    axis2_conf_ctx_t *conf_ctx)
{
    axis2_char_t *sql_update = NULL;
    savan_db_mgr_t *db_mgr = NULL;
    axis2_char_t *id = NULL;
    axis2_char_t *endto = NULL;
    axis2_char_t *notifyto = NULL;
    axis2_char_t *delivery_mode = NULL;
    axis2_char_t *expires = NULL;
    axis2_char_t *filter = NULL;
    axis2_char_t *topic = NULL;
    axis2_char_t *topic_url = NULL;
    axis2_bool_t renewed = AXIS2_FALSE;
    axis2_endpoint_ref_t *endto_epr = NULL;
    axis2_endpoint_ref_t *notifyto_epr = NULL;
    sql_update = AXIS2_MALLOC(env->allocator, 1024);

    id = savan_subscriber_get_id(subscriber, env);
    endto_epr = savan_subscriber_get_end_to(subscriber, env);
    endto = (axis2_char_t *) axis2_endpoint_ref_get_address(endto_epr, env);
    notifyto_epr = savan_subscriber_get_notify_to(subscriber, env);
    notifyto = (axis2_char_t *) axis2_endpoint_ref_get_address(notifyto_epr, 
        env);
    delivery_mode = savan_subscriber_get_delivery_mode(subscriber, env);
    expires = savan_subscriber_get_expires(subscriber, env);
    filter = savan_subscriber_get_filter(subscriber, env);
    topic_url = savan_subscriber_get_topic(subscriber, env);
    AXIS2_LOG_DEBUG(env->log, AXIS2_LOG_SI, "topic_url:%s", topic_url);
    topic = savan_util_get_topic_name_from_topic_url(env, topic_url);
    renewed = savan_subscriber_get_renew_status(subscriber, env);
    db_mgr = savan_db_mgr_create(env, conf_ctx);
    sprintf(sql_update, "update subscriber set end_to='%s', notify_to='%s',"\
        "delivery_mode='%s', expires='%s', filter='%s', topic_name='%s', renewed=%d"\
        " where id='%s'", endto, notifyto, delivery_mode, expires, filter, topic, 
        renewed, id);
    return sql_update;
}

