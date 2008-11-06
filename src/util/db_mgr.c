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
#include <axutil_types.h>
#include <axutil_file_handler.h>
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

AXIS2_EXTERN int
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
    axis2_char_t *dbname)
{
    savan_db_mgr_t *db_mgr = NULL;
    
    db_mgr = AXIS2_MALLOC(env->allocator, sizeof(savan_db_mgr_t));

    db_mgr->dbname = axutil_strdup(env, dbname);

    return db_mgr;
}

AXIS2_EXTERN void AXIS2_CALL
savan_db_mgr_free(
    savan_db_mgr_t *db_mgr,
    const axutil_env_t *env)
{
    AXIS2_LOG_TRACE(env->log, AXIS2_LOG_SI, "[savan] Entry:savan_db_mgr_free");

    if(db_mgr->dbname)
    {
        AXIS2_FREE(env->allocator, db_mgr->dbname);
        db_mgr->dbname = NULL;
    }

    if(db_mgr)
    {
        AXIS2_FREE(env->allocator, db_mgr);
    }

    AXIS2_LOG_TRACE(env->log, AXIS2_LOG_SI, "[savan] Exit:savan_db_mgr_free");
}

AXIS2_EXTERN int 
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

AXIS2_EXTERN int
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

        if(0 == axutil_strcmp(col_name[i], "notify_to") )
        {
            axis2_endpoint_ref_t *notify_epr = NULL;
            notify_epr = axis2_endpoint_ref_create(env, argv[i]);
            savan_subscriber_set_notify_to(subscriber, env, notify_epr);
        }

        if(0 == axutil_strcmp(col_name[i], "delivery_mode") )
        {
            savan_subscriber_set_delivery_mode(subscriber, env, argv[i]);
        }

        if(0 == axutil_strcmp(col_name[i], "expires") )
        {
            savan_subscriber_set_expires(subscriber, env, argv[i]);
        }

        if(0 == axutil_strcmp(col_name[i], "filter") )
        {
            savan_subscriber_set_filter(subscriber, env, argv[i]);
        }

        if(0 == axutil_strcmp(col_name[i], "topic_url") )
        {
            savan_subscriber_set_topic(subscriber, env, argv[i]);
        }

        if(0 == axutil_strcmp(col_name[i], "renewed") )
        {
            savan_subscriber_set_renew_status(subscriber, env, 
                AXIS2_ATOI(argv[i]));
        }
    }

    if(subscriber)
    {
        axutil_array_list_add(subscriber_list, env, subscriber);
    }

    return 0;
}

AXIS2_EXTERN int  
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

	AXIS2_LOG_TRACE(env->log, AXIS2_LOG_SI, "[savan] Entry:savan_db_mgr_subs_retrieve_callback");

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

    AXIS2_LOG_TRACE(env->log, AXIS2_LOG_SI, "[savan] Exit:savan_db_mgr_subs_retrieve_callback");
    return 0;
}

AXIS2_EXTERN axis2_status_t AXIS2_CALL
savan_db_mgr_insert_subscriber(
    const axutil_env_t *env,
    const axis2_char_t *dbname,
    savan_subscriber_t *subscriber)
{
    axis2_char_t sql_insert[1028];
    sqlite3 *dbconn = NULL;
    axis2_char_t *id = NULL;
    axis2_char_t *endto = NULL;
    axis2_char_t *notifyto = NULL;
    axis2_char_t *delivery_mode = NULL;
    axis2_char_t *expires = NULL;
    axis2_char_t *filter = NULL;
    axis2_char_t *topic = NULL;
    int renewed = 0;
    axis2_endpoint_ref_t *endto_epr = NULL;
    axis2_endpoint_ref_t *notifyto_epr = NULL;
    int counter = 1;
    struct sqlite3_stmt* insertqry;

    sprintf(sql_insert, "%s", "insert into subscriber(id");
    
    AXIS2_LOG_DEBUG(env->log, AXIS2_LOG_SI, "sql_insert:%s", sql_insert);

    if(subscriber)
    {
        int i = 0;

        id = savan_subscriber_get_id(subscriber, env);
        endto_epr = savan_subscriber_get_end_to(subscriber, env);
        if(endto_epr)
        {
            endto = (axis2_char_t *) axis2_endpoint_ref_get_address(endto_epr, env);
            if(endto)
            {
                sprintf(sql_insert, "%s%s", sql_insert, ",end_to");
                counter++;
                AXIS2_LOG_DEBUG(env->log, AXIS2_LOG_SI, "sql_insert:%s", sql_insert);
            }
        }

        notifyto_epr = savan_subscriber_get_notify_to(subscriber, env);
        if(notifyto_epr)
        {
            notifyto = (axis2_char_t *) axis2_endpoint_ref_get_address(notifyto_epr, env);
            if(notifyto)
            {
                sprintf(sql_insert, "%s%s", sql_insert, ",notify_to");   
                counter++;
                AXIS2_LOG_DEBUG(env->log, AXIS2_LOG_SI, "sql_insert:%s", sql_insert);
            }
        }

        delivery_mode = savan_subscriber_get_delivery_mode(subscriber, env);
        if(delivery_mode)
        {
            sprintf(sql_insert, "%s%s", sql_insert, ",delivery_mode");   
            counter++;
        }
        expires = savan_subscriber_get_expires(subscriber, env);
        if(expires)
        {
            sprintf(sql_insert, "%s%s", sql_insert, ",expires");   
            counter++;
        }
        filter = savan_subscriber_get_filter(subscriber, env);
        if(filter)
        {
            sprintf(sql_insert, "%s%s", sql_insert, ",filter");   
            counter++;
        }
        topic = savan_subscriber_get_topic(subscriber, env);
        if(topic)
        {
            /*topic = savan_util_get_topic_name_from_topic_url(env, topic_url);*/
            sprintf(sql_insert, "%s%s", sql_insert, ",topic_name");   
            counter++;
        }
        renewed = (int) savan_subscriber_get_renew_status(subscriber, env);
        sprintf(sql_insert, "%s%s", sql_insert, ",renewed");   
        sprintf(sql_insert, "%s%s", sql_insert, ") values(?");
        for(i = 0; i < counter; i++)
        {
            sprintf(sql_insert, "%s%s", sql_insert, ",?");
        }
        sprintf(sql_insert, "%s%s", sql_insert, ");");
    }

    AXIS2_LOG_DEBUG(env->log, AXIS2_LOG_SI, "sql_insert:%s", sql_insert);
    dbconn = (sqlite3 *) savan_db_mgr_get_dbconn(env, dbname);
    if(!dbconn)
    {
        return AXIS2_FAILURE;
    }

    counter = 1;
    if (sqlite3_prepare(dbconn, sql_insert, strlen(sql_insert), &insertqry, NULL))
    {
        AXIS2_LOG_ERROR(env->log, AXIS2_LOG_SI, 
            "[savan] Sql Insert Error: %s", sqlite3_errmsg(dbconn));
    }
    if (sqlite3_bind_text(insertqry, counter, id, strlen(id), SQLITE_STATIC))
    {
        AXIS2_LOG_ERROR(env->log, AXIS2_LOG_SI, 
            "[savan] Sql Insert Error: %s", sqlite3_errmsg(dbconn));
    }

    AXIS2_LOG_DEBUG(env->log, AXIS2_LOG_SI, "[savan] id:%s", id);
    if(endto)
    {
        AXIS2_LOG_DEBUG(env->log, AXIS2_LOG_SI, "[savan] endto:%s", endto);
        counter++;
        if (sqlite3_bind_text(insertqry, counter, endto, strlen(endto), SQLITE_STATIC))
        {
            AXIS2_LOG_ERROR(env->log, AXIS2_LOG_SI, 
                "[savan] Sql Insert Error: %s", sqlite3_errmsg(dbconn));
        }
    }
    if(notifyto)
    {
        AXIS2_LOG_DEBUG(env->log, AXIS2_LOG_SI, "[savan] notifyto:%s", notifyto);
        counter++;
        if (sqlite3_bind_text(insertqry, counter, notifyto, strlen(notifyto), SQLITE_STATIC))
        {
            AXIS2_LOG_ERROR(env->log, AXIS2_LOG_SI, 
                "[savan] Sql Insert Error: %s", sqlite3_errmsg(dbconn));
        }
    }
    if(delivery_mode)
    {
        AXIS2_LOG_DEBUG(env->log, AXIS2_LOG_SI, "[savan] delivery_mode:%s", delivery_mode);
        counter++;
        if (sqlite3_bind_text(insertqry, counter, delivery_mode, strlen(delivery_mode), SQLITE_STATIC))
        {
            AXIS2_LOG_ERROR(env->log, AXIS2_LOG_SI, 
                "[savan] Sql Insert Error: %s", sqlite3_errmsg(dbconn));
        }
    }
    if(expires)
    {
        AXIS2_LOG_DEBUG(env->log, AXIS2_LOG_SI, "[savan] expires:%s", expires);
        counter++;
        if (sqlite3_bind_text(insertqry, counter, expires, strlen(expires), SQLITE_STATIC))
        {
            AXIS2_LOG_ERROR(env->log, AXIS2_LOG_SI, 
                "[savan] Sql Insert Error: %s", sqlite3_errmsg(dbconn));
        }
    }
    if(filter)
    {
        AXIS2_LOG_DEBUG(env->log, AXIS2_LOG_SI, "[savan] filter:%s", filter);
        counter++;
        if (sqlite3_bind_text(insertqry, counter, filter, strlen(filter), SQLITE_STATIC))
        {
            AXIS2_LOG_ERROR(env->log, AXIS2_LOG_SI, 
                "[savan] Sql Insert Error: %s", sqlite3_errmsg(dbconn));
        }
    }
    if(topic)
    {
        AXIS2_LOG_DEBUG(env->log, AXIS2_LOG_SI, "[savan] topic:%s", topic);
        counter++;
        if (sqlite3_bind_text(insertqry, counter, topic, strlen(topic), SQLITE_STATIC))
        {
            AXIS2_LOG_ERROR(env->log, AXIS2_LOG_SI, 
                "[savan] Sql Insert Error: %s", sqlite3_errmsg(dbconn));
        }
    }

    counter++;
    if (sqlite3_bind_int(insertqry, counter, renewed))
    {
        AXIS2_LOG_ERROR(env->log, AXIS2_LOG_SI, 
            "[savan] Sql Insert Error: %s", sqlite3_errmsg(dbconn));
    }

    AXIS2_LOG_DEBUG(env->log, AXIS2_LOG_SI, "[savan] renewed:%d", renewed);
    if (sqlite3_step(insertqry) == SQLITE_DONE)
    {
        AXIS2_LOG_DEBUG(env->log, AXIS2_LOG_SI, 
            "[savan] Subscriber is added to the database");
        sqlite3_reset(insertqry);
    }
    else
    {
        AXIS2_LOG_ERROR(env->log, AXIS2_LOG_SI, 
            "[savan] Sql Insert Error: %s", sqlite3_errmsg(dbconn));
    }
   
    sqlite3_finalize(insertqry);
    sqlite3_close(dbconn);
    return AXIS2_SUCCESS;
}

AXIS2_EXTERN axis2_status_t AXIS2_CALL
savan_db_mgr_update_subscriber(
    const axutil_env_t *env,
    const axis2_char_t *dbname,
    savan_subscriber_t *subscriber)
{
    axis2_char_t *sql_update = NULL;
    sqlite3 *dbconn = NULL;
    axis2_char_t *id = NULL;
    axis2_char_t *endto = NULL;
    axis2_char_t *notifyto = NULL;
    axis2_char_t *delivery_mode = NULL;
    axis2_char_t *expires = NULL;
    axis2_char_t *filter = NULL;
    axis2_char_t *topic = NULL;
    axis2_char_t *topic_url = NULL;
    int renewed = 0;
    axis2_endpoint_ref_t *endto_epr = NULL;
    axis2_endpoint_ref_t *notifyto_epr = NULL;
    int counter = 1;
    struct sqlite3_stmt* updateqry;

    sql_update = AXIS2_MALLOC(env->allocator, 1028);
    sprintf(sql_update, "%s", "update subscriber set ");
            AXIS2_LOG_DEBUG(env->log, AXIS2_LOG_SI, "sql_update:%s", sql_update);

    if(subscriber)
    {
        id = savan_subscriber_get_id(subscriber, env);
        endto_epr = savan_subscriber_get_end_to(subscriber, env);
        if(endto_epr)
        {
            endto = (axis2_char_t *) axis2_endpoint_ref_get_address(endto_epr, env);
            if(endto)
            {
                sprintf(sql_update, "%s%s", sql_update, "end_to=?, ");
                counter++;
                AXIS2_LOG_DEBUG(env->log, AXIS2_LOG_SI, "sql_update:%s", sql_update);
            }
        }

        notifyto_epr = savan_subscriber_get_notify_to(subscriber, env);
        if(notifyto_epr)
        {
            notifyto = (axis2_char_t *) axis2_endpoint_ref_get_address(notifyto_epr, env);
            if(notifyto)
            {
                sprintf(sql_update, "%s%s", sql_update, "notify_to=?, ");   
                counter++;
                AXIS2_LOG_DEBUG(env->log, AXIS2_LOG_SI, "sql_update:%s", sql_update);
            }
        }

        delivery_mode = savan_subscriber_get_delivery_mode(subscriber, env);
        if(delivery_mode)
        {
            sprintf(sql_update, "%s%s", sql_update, "delivery_mode=?, ");   
            counter++;
        }

        expires = savan_subscriber_get_expires(subscriber, env);
        if(expires)
        {
            sprintf(sql_update, "%s%s", sql_update, "expires=?, ");   
            counter++;
        }

        filter = savan_subscriber_get_filter(subscriber, env);
        if(filter)
        {
            sprintf(sql_update, "%s%s", sql_update, "filter=?, ");   
            counter++;
        }

        topic_url = savan_subscriber_get_topic(subscriber, env);
        if(topic_url)
        {
            topic = savan_util_get_topic_name_from_topic_url(env, topic_url);
            sprintf(sql_update, "%s%s", sql_update, "topic_name=?, ");   
            counter++;
        }

        renewed = (int) savan_subscriber_get_renew_status(subscriber, env);
        sprintf(sql_update, "%s%s", sql_update, "renewed=? where id=?;");   
    }

    AXIS2_LOG_DEBUG(env->log, AXIS2_LOG_SI, "sql_update:%s", sql_update);
    dbconn = (sqlite3 *) savan_db_mgr_get_dbconn(env, dbname);
    if(!dbconn)
    {
        return AXIS2_FAILURE;
    }

    counter = 0;
    if (sqlite3_prepare(dbconn, sql_update, strlen(sql_update), &updateqry, NULL))
    {
        AXIS2_LOG_ERROR(env->log, AXIS2_LOG_SI, 
            "[savan] Sql Update Error: %s", sqlite3_errmsg(dbconn));
    }

    if(endto)
    {
        AXIS2_LOG_DEBUG(env->log, AXIS2_LOG_SI, "[savan] endto:%s", endto);
        counter++;
        if (sqlite3_bind_text(updateqry, counter, endto, strlen(endto), SQLITE_STATIC))
        {
            AXIS2_LOG_ERROR(env->log, AXIS2_LOG_SI, 
                "[savan] Sql Update Error: %s", sqlite3_errmsg(dbconn));
        }
    }

    if(notifyto)
    {
        AXIS2_LOG_DEBUG(env->log, AXIS2_LOG_SI, "[savan] notifyto:%s", notifyto);
        counter++;
        if (sqlite3_bind_text(updateqry, counter, notifyto, strlen(notifyto), SQLITE_STATIC))
        {
            AXIS2_LOG_ERROR(env->log, AXIS2_LOG_SI, 
                "[savan] Sql Update Error: %s", sqlite3_errmsg(dbconn));
        }
    }

    if(delivery_mode)
    {
        AXIS2_LOG_DEBUG(env->log, AXIS2_LOG_SI, "[savan] delivery_mode:%s", delivery_mode);
        counter++;
        if (sqlite3_bind_text(updateqry, counter, delivery_mode, strlen(delivery_mode), SQLITE_STATIC))
        {
            AXIS2_LOG_ERROR(env->log, AXIS2_LOG_SI, 
                "[savan] Sql Update Error: %s", sqlite3_errmsg(dbconn));
        }
    }

    if(expires)
    {
        AXIS2_LOG_DEBUG(env->log, AXIS2_LOG_SI, "[savan] expires:%s", expires);
        counter++;
        if (sqlite3_bind_text(updateqry, counter, expires, strlen(expires), SQLITE_STATIC))
        {
            AXIS2_LOG_ERROR(env->log, AXIS2_LOG_SI, 
                "[savan] Sql Update Error: %s", sqlite3_errmsg(dbconn));
        }
    }

    if(filter)
    {
        AXIS2_LOG_DEBUG(env->log, AXIS2_LOG_SI, "[savan] filter:%s", filter);
        counter++;
        if (sqlite3_bind_text(updateqry, counter, filter, strlen(filter), SQLITE_STATIC))
        {
            AXIS2_LOG_ERROR(env->log, AXIS2_LOG_SI, 
                "[savan] Sql Update Error: %s", sqlite3_errmsg(dbconn));
        }
    }

    if(topic)
    {
        AXIS2_LOG_DEBUG(env->log, AXIS2_LOG_SI, "[savan] topic:%s", topic);
        counter++;
        if (sqlite3_bind_text(updateqry, counter, topic, strlen(topic), SQLITE_STATIC))
        {
            AXIS2_LOG_ERROR(env->log, AXIS2_LOG_SI, 
                "[savan] Sql Update Error: %s", sqlite3_errmsg(dbconn));
        }
    }

    counter++;
    if (sqlite3_bind_int(updateqry, counter, renewed))
    {
        AXIS2_LOG_ERROR(env->log, AXIS2_LOG_SI, 
            "[savan] Sql Update Error: %s", sqlite3_errmsg(dbconn));
    }

    AXIS2_LOG_DEBUG(env->log, AXIS2_LOG_SI, "[savan] renewed:%d", renewed);
    counter++;
    if (sqlite3_bind_text(updateqry, counter, id, strlen(id), SQLITE_STATIC))
    {
        AXIS2_LOG_ERROR(env->log, AXIS2_LOG_SI, 
            "[savan] Sql Update Error: %s", sqlite3_errmsg(dbconn));
    }

    AXIS2_LOG_DEBUG(env->log, AXIS2_LOG_SI, "[savan] id:%s", id);
    if (sqlite3_step(updateqry) == SQLITE_DONE)
    {
        AXIS2_LOG_DEBUG(env->log, AXIS2_LOG_SI, 
            "[savan] Subscriber is updated to the database");
        sqlite3_reset(updateqry);
    }
    else
    {
        AXIS2_LOG_ERROR(env->log, AXIS2_LOG_SI, 
            "[savan] Sql Update Error: %s", sqlite3_errmsg(dbconn));
    }
   
    AXIS2_FREE(env->allocator, sql_update);
    sqlite3_finalize(updateqry);
    sqlite3_close(dbconn);
    return AXIS2_SUCCESS;
}

AXIS2_EXTERN axis2_status_t AXIS2_CALL
savan_db_mgr_insert_topic(
    const axutil_env_t *env,
    const axis2_char_t *dbname,
    axis2_char_t *topic_name,
    axis2_char_t *topic_url)
{
    axis2_char_t *sql_insert = NULL;
    sqlite3 *dbconn = NULL;
    struct sqlite3_stmt* insertqry;

    sql_insert = "insert into topic(topic_name, topic_url) values(?, ?);";
    dbconn = (sqlite3 *) savan_db_mgr_get_dbconn(env, dbname);
    if(!dbconn)
    {
        return AXIS2_FAILURE;
    }

    if (sqlite3_prepare(dbconn, sql_insert, strlen(sql_insert), &insertqry, NULL))
    {
        AXIS2_LOG_ERROR(env->log, AXIS2_LOG_SI, "[savan] Sql Insert Error: %s", 
                sqlite3_errmsg(dbconn));
    }

    if (sqlite3_bind_text(insertqry, 1, topic_name, strlen(topic_name), SQLITE_STATIC))
    {
        AXIS2_LOG_ERROR(env->log, AXIS2_LOG_SI, "[savan] Sql Insert Error: %s", 
                sqlite3_errmsg(dbconn));
    }

    if(topic_url)
    {
        if (sqlite3_bind_text(insertqry, 2, topic_url, strlen(topic_url), SQLITE_STATIC))
        {
            AXIS2_LOG_ERROR(env->log, AXIS2_LOG_SI, "[savan] Sql Insert Error: %s", 
                sqlite3_errmsg(dbconn));
        }
    }

    if (sqlite3_step(insertqry) == SQLITE_DONE)
    {
        sqlite3_reset(insertqry);
    }
    
    sqlite3_finalize(insertqry);
    sqlite3_close(dbconn);

    return AXIS2_SUCCESS;
}


AXIS2_EXTERN axis2_status_t AXIS2_CALL
savan_db_mgr_remove(
    const axutil_env_t *env,
    const axis2_char_t *dbname,
    axis2_char_t *sql_stmt_remove)
{
    axis2_char_t *error_msg = NULL;
    sqlite3 *dbconn = NULL;
    int rc = -1;

    dbconn = (sqlite3 *) savan_db_mgr_get_dbconn(env, dbname);
    if(!dbconn)
    {
        return AXIS2_FAILURE;
    }

    rc = sqlite3_exec(dbconn, "BEGIN;", 0, 0, &error_msg);
    if(rc == SQLITE_BUSY)
    {
        rc = savan_db_mgr_busy_handler(dbconn, "BEGIN;", 0, 0, &error_msg, rc);
    }

    rc = sqlite3_exec(dbconn, sql_stmt_remove, 0, 0, &error_msg);
    if(rc == SQLITE_BUSY)
    {
        rc = savan_db_mgr_busy_handler(dbconn, sql_stmt_remove, 0, 0, &error_msg, rc);
    }
    if(rc != SQLITE_OK )
    {
        rc = sqlite3_exec(dbconn, "ROLLBACK;", 0, 0, &error_msg);
        if(rc == SQLITE_BUSY)
        {
            rc = savan_db_mgr_busy_handler(dbconn, "ROLLBACK;", 0, 0, &error_msg, rc);
        }
        AXIS2_LOG_ERROR(env->log, AXIS2_LOG_SI, 
            "[savan] Error Sql Remove Stmt: %s. Sql Error: %s", sql_stmt_remove, error_msg);
        sqlite3_free(error_msg);
        sqlite3_close(dbconn);
        return AXIS2_FAILURE;
    }

    rc = sqlite3_exec(dbconn, "COMMIT;", 0, 0, &error_msg);
    if(rc == SQLITE_BUSY)
    {
        rc = savan_db_mgr_busy_handler(dbconn, "COMMIT;", 0, 0, &error_msg, rc);
    }

    sqlite3_close(dbconn);
    return AXIS2_SUCCESS;
}

AXIS2_EXTERN axis2_status_t AXIS2_CALL
savan_db_mgr_update(
    const axutil_env_t *env,
    const axis2_char_t *dbname,
    axis2_char_t *sql_stmt_update)
{
    sqlite3 *dbconn = NULL;
    axis2_char_t *error_msg = NULL;
    int rc = -1;
    
    AXIS2_LOG_TRACE(env->log, AXIS2_LOG_SI, "[savan] Entry:savan_db_mgr_update");
    dbconn = (sqlite3 *) savan_db_mgr_get_dbconn(env, dbname);
    if(!dbconn)
    {
        return AXIS2_FAILURE;
    }

    rc = sqlite3_exec(dbconn, "BEGIN;", 0, 0, &error_msg);
    if(rc == SQLITE_BUSY)
    {
        rc = savan_db_mgr_busy_handler(dbconn, "BEGIN;", 0, 0, &error_msg, rc);
    }

    rc = sqlite3_exec(dbconn, sql_stmt_update, 0, 0, &error_msg);
    if(rc == SQLITE_BUSY)
    {
        rc = savan_db_mgr_busy_handler(dbconn, sql_stmt_update, 0, 0, &error_msg, rc);
    }
    if(rc != SQLITE_OK )
    {
        rc = sqlite3_exec(dbconn, "ROLLBACK;", 0, 0, &error_msg);
        if(rc == SQLITE_BUSY)
        {
            rc = savan_db_mgr_busy_handler(dbconn, "ROLLBACK;", 0, 0, &error_msg, rc);
        }

        AXIS2_LOG_ERROR(env->log, AXIS2_LOG_SI, 
            "[savan] Error Sql Update Stmt:%s", sql_stmt_update);
        AXIS2_LOG_ERROR(env->log, AXIS2_LOG_SI, "[savan] Sql Update Error:%s", error_msg);
        sqlite3_free(error_msg);
        sqlite3_close(dbconn);
        return AXIS2_FAILURE;
    }

    rc = sqlite3_exec(dbconn, "COMMIT;", 0, 0, &error_msg);
    if(rc == SQLITE_BUSY)
    {
        rc = savan_db_mgr_busy_handler(dbconn, "COMMIT;", 0, 0, &error_msg, rc);
    }
    sqlite3_close(dbconn);

    AXIS2_LOG_TRACE(env->log, AXIS2_LOG_SI, "[savan] Exit:savan_db_mgr_update");
    return AXIS2_SUCCESS;
}

AXIS2_EXTERN savan_subscriber_t *AXIS2_CALL
savan_db_mgr_retrieve(
    const axutil_env_t *env,
    const axis2_char_t *dbname,
    int (*retrieve_func)(void *, int, char **, char **),
    axis2_char_t *sql_stmt_retrieve)
{
    savan_db_mgr_args_t *args = NULL;
    axis2_char_t *error_msg = NULL;
    savan_subscriber_t *subscriber = NULL;
    sqlite3 *dbconn = NULL;
    int rc = -1;

    AXIS2_LOG_TRACE(env->log, AXIS2_LOG_SI, "[savan] Entry:savan_db_mgr_retrieve");

    dbconn = (sqlite3 *) savan_db_mgr_get_dbconn(env, dbname);
    if(!dbconn)
    {
        return NULL;
    }

    rc = sqlite3_exec(dbconn, "BEGIN READ_ONLY;", 0, 0, &error_msg);
    if(rc == SQLITE_BUSY)
    {
        rc = savan_db_mgr_busy_handler(dbconn, "BEGIN READ_ONLY;", 0, 0, &error_msg, rc);
    }

    args = AXIS2_MALLOC(env->allocator, sizeof(savan_db_mgr_args_t));
    args->env = (axutil_env_t*)env;
    args->data = NULL;

    rc = sqlite3_exec(dbconn, sql_stmt_retrieve, retrieve_func, args, &error_msg);
    if(rc == SQLITE_BUSY)
    {
        rc = savan_db_mgr_busy_handler(dbconn, sql_stmt_retrieve, retrieve_func, args, &error_msg, 
                rc);
    }

    if(rc != SQLITE_OK )
    {
        rc = sqlite3_exec(dbconn, "ROLLBACK;", 0, 0, &error_msg);
        if(rc == SQLITE_BUSY)
        {
            rc = savan_db_mgr_busy_handler(dbconn,
                "ROLLBACK;", 0, 0, &error_msg, rc);
        }
        AXIS2_LOG_ERROR(env->log, AXIS2_LOG_SI, 
            "[savan] Error Sql Retrieve Stmt: %s. Sql Error: %s", sql_stmt_retrieve, error_msg);
        sqlite3_free(error_msg);
        sqlite3_close(dbconn);
        return AXIS2_FALSE;
    }

    if(args->data)
    {
        subscriber = (savan_subscriber_t *) args->data;
    }

    if(args)
    {
        AXIS2_FREE(env->allocator, args);
    }

    rc = sqlite3_exec(dbconn, "COMMIT;", 0, 0, &error_msg);
    if(rc == SQLITE_BUSY)
    {
        rc = savan_db_mgr_busy_handler(dbconn, "COMMIT;", 0, 0, &error_msg, rc);
    }
    sqlite3_close(dbconn);

    AXIS2_LOG_TRACE(env->log, AXIS2_LOG_SI, "[savan] Exit:savan_db_mgr_retrieve");
    return subscriber;
}

AXIS2_EXTERN axutil_array_list_t * AXIS2_CALL
savan_db_mgr_retrieve_all(
    const axutil_env_t *env,
    const axis2_char_t *dbname,
    int (*find_func)(void *, int, char **, char **),
    axis2_char_t *sql_stmt_find)
{
    savan_db_mgr_args_t *args = NULL;
    axutil_array_list_t *data_list = NULL;
    int rc = -1;
    sqlite3 *dbconn = NULL;
    axis2_char_t *error_msg = NULL;
    data_list = axutil_array_list_create(env, 0);
    if(!data_list)
    {
        AXIS2_ERROR_SET(env->error, AXIS2_ERROR_NO_MEMORY, AXIS2_FAILURE);
        return NULL;
    }
    args = AXIS2_MALLOC(env->allocator, sizeof(savan_db_mgr_args_t));
    args->env = (axutil_env_t*)env;
    args->data = NULL;
    dbconn = (sqlite3 *) savan_db_mgr_get_dbconn(env, dbname);
    if(!dbconn)
    {
        return NULL;
    }
    rc = sqlite3_exec(dbconn, "BEGIN READ_ONLY;", 0, 0, &error_msg);
    if(rc == SQLITE_BUSY)
    {
        rc = savan_db_mgr_busy_handler(dbconn, "BEGIN READ_ONLY;", 0, 0, &error_msg, rc);
    }
    rc = sqlite3_exec(dbconn, sql_stmt_find, find_func, args, &error_msg);

    if(rc == SQLITE_BUSY)
    {
        rc = savan_db_mgr_busy_handler(dbconn, sql_stmt_find, 
            find_func, args, &error_msg, rc);
    }

    if(args->data)
    {
        data_list = (axutil_array_list_t *) args->data;
    }

    if(rc != SQLITE_OK )
    {
        rc = sqlite3_exec(dbconn, "ROLLBACK;", 0, 0, &error_msg);
        if(rc == SQLITE_BUSY)
        {
            rc = savan_db_mgr_busy_handler(dbconn,
                "ROLLBACK;", 0, 0, &error_msg, rc);
        }

        if(data_list)
        {
            axutil_array_list_free(data_list, env);
        }

        if(args)
        {
            AXIS2_FREE(env->allocator, args);
        }

        AXIS2_LOG_ERROR(env->log, AXIS2_LOG_SI, "[savan] Error Sql Retrieve All Stmt:%s", 
                sql_stmt_find); 

        AXIS2_LOG_ERROR(env->log, AXIS2_LOG_SI, "[savan] Sql Retrieve All Error:%s", error_msg);
        sqlite3_free(error_msg);
        sqlite3_close(dbconn);
        return NULL;
    }

    if(args)
    {
        AXIS2_FREE(env->allocator, args);
    }

    rc = sqlite3_exec(dbconn, "COMMIT;", 0, 0, &error_msg);
    if(rc == SQLITE_BUSY)
    {
        rc = savan_db_mgr_busy_handler(dbconn,
            "COMMIT;", 0, 0, &error_msg, rc);
    }

    sqlite3_close(dbconn);
    return data_list;
}

AXIS2_EXTERN axis2_status_t AXIS2_CALL
savan_db_mgr_create_db(
    const axutil_env_t *env,
    const axis2_char_t *dbname)
{
    int rc = -1;
    axis2_char_t *error_msg = NULL;
    sqlite3 *dbconn = NULL;
    axis2_char_t *sql_stmt1 = NULL;
    axis2_char_t *sql_stmt2 = NULL;
    axis2_status_t status = AXIS2_FAILURE;

    AXIS2_LOG_TRACE(env->log, AXIS2_LOG_SI, "[savan] Entry:savan_db_mgr_create_db");

    if(AXIS2_SUCCESS == axutil_file_handler_access(dbname, AXIS2_F_OK))
    {
        AXIS2_LOG_DEBUG(env->log, AXIS2_LOG_SI, "[savan] Database %s already created.", dbname);
        return AXIS2_SUCCESS;
    }

    dbconn = savan_db_mgr_get_dbconn(env, dbname);

    #if !defined(WIN32)
    {
        axis2_char_t permission_str[256];
        sprintf(permission_str, "chmod 777 %s", dbname);
        system(permission_str);
    }
    #endif

    sql_stmt1 = "create table if not exists topic(topic_name varchar(100) "\
                 "primary key, topic_url varchar(200))";
    sql_stmt2 = "create table if not exists subscriber(id varchar(100) "\
                  "primary key, end_to varchar(200), notify_to varchar(200), "\
                  "delivery_mode varchar(100), expires varchar(100), "\
                  "filter varchar(200), topic_name varchar(100), "\
                  "renewed boolean)";

    if(dbconn)
    {
        rc = sqlite3_exec(dbconn, sql_stmt1, NULL, 0, &error_msg);
        if( rc != SQLITE_OK )
        {
            AXIS2_LOG_ERROR(env->log, AXIS2_LOG_SI, 
                "[savan] Error creating database table topic; SQL Error: %s", error_msg);
            sqlite3_free(error_msg);
            sqlite3_close(dbconn);
            return AXIS2_FAILURE;
        }

        rc = sqlite3_exec(dbconn, sql_stmt2, NULL, 0, &error_msg);
        if( rc != SQLITE_OK )
        {
            AXIS2_LOG_ERROR(env->log, AXIS2_LOG_SI, 
                    "[savan] Error creating database table subscriber; SQL Error: %s", error_msg);

            sqlite3_free(error_msg);
            sqlite3_close(dbconn);
            return AXIS2_FAILURE;
        }
        sqlite3_close(dbconn);
        status = AXIS2_SUCCESS;
    }
    else
    {
        AXIS2_LOG_ERROR(env->log, AXIS2_LOG_SI, "[savan] Database %s creation failed", dbname);
        return AXIS2_FAILURE;
    }

    AXIS2_LOG_TRACE(env->log, AXIS2_LOG_SI, "[savan] Exit:savan_db_mgr_create_db");

    return status;
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
    const axutil_env_t *env,
    const axis2_char_t *dbname)
{
    int rc = -1;
    sqlite3 *dbconn = NULL;

    rc = sqlite3_open(dbname, &dbconn);
    if(rc != SQLITE_OK)
    {
        AXIS2_LOG_ERROR(env->log, AXIS2_LOG_SI, "[savan] Can't open database: %s sqlite error: %s\n", 
                dbname, sqlite3_errmsg(dbconn));

        sqlite3_close(dbconn);
        dbconn = NULL;
        return NULL;
    }

    return dbconn;
}

axis2_char_t *AXIS2_CALL
savan_db_mgr_create_update_sql(
    const axutil_env_t *env,
    savan_subscriber_t *subscriber)
{
    axis2_char_t *sql_update = NULL;
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
    notifyto = (axis2_char_t *) axis2_endpoint_ref_get_address(notifyto_epr, env);
    delivery_mode = savan_subscriber_get_delivery_mode(subscriber, env);
    expires = savan_subscriber_get_expires(subscriber, env);
    filter = savan_subscriber_get_filter(subscriber, env);
    topic_url = savan_subscriber_get_topic(subscriber, env);
    if(topic_url)
    {
        AXIS2_LOG_DEBUG(env->log, AXIS2_LOG_SI, "topic_url:%s", topic_url);
        topic = savan_util_get_topic_name_from_topic_url(env, topic_url);
    }

    renewed = savan_subscriber_get_renew_status(subscriber, env);

    sprintf(sql_update, "update subscriber set end_to='%s', notify_to='%s',"\
        "delivery_mode='%s', expires='%s', filter='%s', topic_name='%s', renewed=%d"\
        " where id='%s'", endto, notifyto, delivery_mode, expires, filter, topic, 
        renewed, id);

    return sql_update;
}

