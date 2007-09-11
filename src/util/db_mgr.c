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

AXIS2_EXTERN savan_db_mgr_t * AXIS2_CALL
savan_db_mgr_create(
    const axutil_env_t *env,
    axis2_conf_ctx_t *conf_ctx)
{
    savan_db_mgr_t *db_mgr = NULL;
    
    AXIS2_ENV_CHECK(env, NULL);
    db_mgr = AXIS2_MALLOC(env->allocator, sizeof(savan_db_mgr_t));

    db_mgr->mutex = axutil_thread_mutex_create(env->allocator,
        AXIS2_THREAD_MUTEX_DEFAULT);
    return db_mgr;
}

void AXIS2_CALL
savan_db_mgr_free(
    savan_db_mgr_t *db_mgr,
    const axutil_env_t *env)
{
    AXIS2_LOG_DEBUG(env->log, AXIS2_LOG_SI,
        "[SAVAN] Start:savan_db_mgr_free");

    if(db_mgr->mutex)
    {
        axutil_thread_mutex_destroy(db_mgr->mutex);
        db_mgr->mutex = NULL;
    }
    if(db_mgr)
    {
        AXIS2_FREE(env->allocator, db_mgr);
    }

    AXIS2_LOG_DEBUG(env->log, AXIS2_LOG_SI,
        "[SAVAN] Exit:savan_db_mgr_free");
}

int AXIS2_CALL
savan_db_mgr_find_callback(
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
        if(0 == axutil_strcmp(col_name[i], "topic"))
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

int AXIS2_CALL 
savan_db_mgr_retrieve_callback(
    void *not_used, 
    int argc, 
    char **argv, 
    char **col_name)
{
    int i = 0;
    savan_db_mgr_args_t *args = (savan_db_mgr_args_t *) not_used;
    const axutil_env_t *env = args->env;
    savan_subscriber_t *subscriber = (savan_subscriber_t *) args->data;
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
        if(0 == axutil_strcmp(col_name[i], "topic"))
        {
            savan_subscriber_set_topic(subscriber, env, argv[i]);
        }
        if(0 == axutil_strcmp(col_name[i], "renewed"))
        {
            savan_subscriber_set_renew_status(subscriber, env, 
                AXIS2_ATOI(argv[i]));
        }
    }
    return 0;
}

axis2_bool_t AXIS2_CALL
savan_db_mgr_insert(
    savan_db_mgr_t *db_mgr,
    const axutil_env_t *env,
    savan_subscriber_t *subscriber,
    axis2_char_t *sql_stmt_insert)
{
    axis2_char_t *error_msg = NULL;
    int rc = -1;
    sqlite3 *dbconn = NULL;
    AXIS2_ENV_CHECK(env, AXIS2_FALSE);
   
    axutil_thread_mutex_lock(db_mgr->mutex);
    dbconn = (sqlite3 *) savan_db_mgr_get_dbconn(
        db_mgr, env);
    if(!dbconn)
        return AXIS2_FALSE;
    rc = sqlite3_exec(dbconn, sql_stmt_insert, 0, 0, &error_msg);
    if(rc == SQLITE_BUSY)
        rc = savan_db_mgr_busy_handler(dbconn, sql_stmt_insert, 
            0, 0, &error_msg, rc);
    if( rc != SQLITE_OK )
    {
        axutil_thread_mutex_unlock(db_mgr->mutex);
        AXIS2_LOG_ERROR(env->log, AXIS2_LOG_SI, "sql stmt: %s. sql error: %s", 
            sql_stmt_insert, error_msg);
        printf("sql_stmt_insert:%s\n", sql_stmt_insert);
        printf("insert error_msg:%s\n", error_msg);
        sqlite3_free(error_msg);
        return AXIS2_FALSE;
    }
    axutil_thread_mutex_unlock(db_mgr->mutex);
    return AXIS2_TRUE;
}

axis2_bool_t AXIS2_CALL
savan_db_mgr_remove(
    savan_db_mgr_t *db_mgr,
    const axutil_env_t *env,
    axis2_char_t *sql_stmt_remove)
{
    axis2_char_t *error_msg = NULL;
    sqlite3 *dbconn = NULL;
    int rc = -1;
    AXIS2_ENV_CHECK(env, AXIS2_FALSE);
    axutil_thread_mutex_lock(db_mgr->mutex);
    dbconn = (sqlite3 *) savan_db_mgr_get_dbconn(
        db_mgr, env);
    if(!dbconn)
        return AXIS2_FALSE;
    rc = sqlite3_exec(dbconn, sql_stmt_remove, 0, 0, &error_msg);
    if(rc == SQLITE_BUSY)
        rc = savan_db_mgr_busy_handler(dbconn, sql_stmt_remove, 
            0, 0, &error_msg, rc);
    if(rc != SQLITE_OK )
    {
        axutil_thread_mutex_unlock(db_mgr->mutex);
        AXIS2_LOG_ERROR(env->log, AXIS2_LOG_SI, "sql stmt: %s. sql error: %s",
            sql_stmt_remove, error_msg);
        printf("sql_stmt_remove:%s\n", sql_stmt_remove);
        printf("remove error_msg:%s\n", error_msg);
        sqlite3_free(error_msg);
        return AXIS2_FALSE;
    }
    axutil_thread_mutex_unlock(db_mgr->mutex);
    return AXIS2_TRUE;
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
    AXIS2_ENV_CHECK(env, AXIS2_FALSE);
    axutil_thread_mutex_lock(db_mgr->mutex);
    dbconn = (sqlite3 *) savan_db_mgr_get_dbconn(
        db_mgr, env);
    if(!dbconn)
        return NULL;
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
        axutil_thread_mutex_unlock(db_mgr->mutex);
        AXIS2_LOG_ERROR(env->log, AXIS2_LOG_SI, "sql stmt: %s. sql error: %s",
            sql_stmt_retrieve, error_msg);
        printf("sql_stmt_retrieve:%s\n", sql_stmt_retrieve);
        printf("retrieve error_msg:%s\n", error_msg);
        sqlite3_free(error_msg);
        return AXIS2_FALSE;
    }
    if(args->data)
        subscriber = (savan_subscriber_t *) args->data;
    if(args)
        AXIS2_FREE(env->allocator, args);
    axutil_thread_mutex_unlock(db_mgr->mutex);
    return subscriber;
}


axis2_bool_t AXIS2_CALL
savan_db_mgr_update(
    savan_db_mgr_t *db_mgr,
    const axutil_env_t *env,
    savan_subscriber_t *subscriber,
    axis2_char_t *sql_stmt_update)
{
    sqlite3 *dbconn = NULL;
    axis2_char_t *error_msg = NULL;
    int rc = -1;
    AXIS2_ENV_CHECK(env, AXIS2_FALSE);
    axutil_thread_mutex_lock(db_mgr->mutex);
    dbconn = (sqlite3 *) savan_db_mgr_get_dbconn(
        db_mgr, env);
    if(!dbconn)
        return AXIS2_FALSE;
    rc = sqlite3_exec(dbconn, sql_stmt_update, 0, 0, &error_msg);
    if(rc == SQLITE_BUSY)
        rc = savan_db_mgr_busy_handler(dbconn, sql_stmt_update, 
            0, 0, &error_msg, rc);
    if(rc != SQLITE_OK )
    {
        axutil_thread_mutex_unlock(db_mgr->mutex);
        AXIS2_LOG_ERROR(env->log, AXIS2_LOG_SI, "sql error %s", 
            error_msg);
        printf("sql_stmt_update:%s\n", sql_stmt_update);
        printf("update error_msg:%s\n", error_msg);
        sqlite3_free(error_msg);
        return AXIS2_FALSE;
    }
    axutil_thread_mutex_unlock(db_mgr->mutex);
    return AXIS2_TRUE;
}

axutil_array_list_t *AXIS2_CALL
savan_db_mgr_retrieve_all(
    savan_db_mgr_t *db_mgr,
    const axutil_env_t *env,
    savan_subscriber_t *subscriber,
    int (*find_func)(void *, int, char **, char **),
    axis2_char_t *sql_stmt_find)
{
    savan_db_mgr_args_t *args = NULL;
    axutil_array_list_t *subscriber_list = NULL;
    int rc = -1;
    sqlite3 *dbconn = NULL;
    axis2_char_t *error_msg = NULL;
    AXIS2_ENV_CHECK(env, AXIS2_FALSE);
    subscriber_list = axutil_array_list_create(env, 0);
    if(!subscriber_list)
    {
        AXIS2_ERROR_SET(env->error, AXIS2_ERROR_NO_MEMORY, AXIS2_FAILURE);
        return NULL;
    }
    axutil_thread_mutex_lock(db_mgr->mutex);
    args = AXIS2_MALLOC(env->allocator, sizeof(savan_db_mgr_args_t));
    args->env = (axutil_env_t*)env;
    args->data = NULL;
    dbconn = (sqlite3 *) savan_db_mgr_get_dbconn(
        db_mgr, env);
    if(!dbconn)
    {
        axutil_thread_mutex_unlock(db_mgr->mutex);
        return NULL;
    }
    rc = sqlite3_exec(dbconn, sql_stmt_find, find_func, args, 
        &error_msg);
    if(rc == SQLITE_BUSY)
        rc = savan_db_mgr_busy_handler(dbconn, sql_stmt_find, 
            find_func, args, &error_msg, rc);
    if(args->data)
        subscriber_list = (axutil_array_list_t *) args->data;
    if(rc != SQLITE_OK )
    {
        axutil_thread_mutex_unlock(db_mgr->mutex);
        if(subscriber_list)
            axutil_array_list_free(subscriber_list, env);
        if(args)
            AXIS2_FREE(env->allocator, args);
        AXIS2_LOG_ERROR(env->log, AXIS2_LOG_SI, "sql error %s", 
            error_msg);
        printf("sql_stmt_find:%s\n", sql_stmt_find);
        printf("retrieve error_msg:%s\n", error_msg);
        sqlite3_free(error_msg);
        return NULL;
    }
    if(args)
        AXIS2_FREE(env->allocator, args);
    axutil_thread_mutex_unlock(db_mgr->mutex);
    /* Now we have a point-in-time view of the subscriber_list, lock them all.*/
    axutil_thread_mutex_unlock(db_mgr->mutex);
    return subscriber_list;
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
    rc = sqlite3_open(path, &(db_mgr->dbconn));
    if(rc != SQLITE_OK)
    {
        AXIS2_LOG_ERROR(env->log, AXIS2_LOG_SI, "Can't open database: %s"
            " sqlite error: %s\n", path, sqlite3_errmsg(db_mgr->dbconn));
        sqlite3_close(db_mgr->dbconn);
        return NULL;
    }
    return db_mgr->dbconn;
}

