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

#ifndef SAVAN_DB_MGR_H
#define SAVAN_DB_MGR_H

/**
 * @file savan_db_mgr.h
 * @brief Savan Database Manager Interface
 */

#include <platforms/axutil_platform_auto_sense.h>
#include <axutil_allocator.h>
#include <axutil_env.h>
#include <axutil_error.h>
#include <axutil_string.h>
#include <axutil_utils.h>
#include <axutil_array_list.h>
#include <axis2_conf_ctx.h>
#include <savan_subscriber.h>
#include <sqlite3.h>

#ifdef __cplusplus
extern "C"
{
#endif

typedef AXIS2_DECLARE_DATA struct savan_db_mgr_args
{
    const axutil_env_t *env;
    void *data;
} savan_db_mgr_args_t;

/** 
 * @brief Savan Database Manager Struct Impl
 *   Savan Database Manager
 */
typedef struct savan_db_mgr
{
    axis2_conf_ctx_t *conf_ctx;
    axutil_thread_mutex_t *mutex;
    sqlite3* dbconn;
}savan_db_mgr_t;

AXIS2_EXTERN savan_db_mgr_t * AXIS2_CALL
savan_db_mgr_create(
    const axutil_env_t *env,
    axis2_conf_ctx_t *conf_ctx);

void AXIS2_CALL
savan_db_mgr_free(
    savan_db_mgr_t *db_mgr,
    const axutil_env_t *env);

int AXIS2_CALL
savan_db_mgr_find_callback(
    void *not_used, 
    int argc, 
    char **argv, 
    char **col_name);

int AXIS2_CALL
savan_db_mgr_retrieve_callback(
    void *not_used, 
    int argc, 
    char **argv, 
    char **col_name);

axis2_bool_t AXIS2_CALL
savan_db_mgr_insert(
    savan_db_mgr_t *db_mgr,
    const axutil_env_t *env,
    savan_subscriber_t *subscriber,
    axis2_char_t *sql_stmt_insert);

axis2_bool_t AXIS2_CALL
savan_db_mgr_remove(
    savan_db_mgr_t *db_mgr,
    const axutil_env_t *env,
    axis2_char_t *sql_stmt_remove);

savan_subscriber_t *AXIS2_CALL
savan_db_mgr_retrieve(
    savan_db_mgr_t *db_mgr,
    const axutil_env_t *env,
    int (*retrieve_func)(void *, int, char **, char **),
    axis2_char_t *sql_stmt_retrieve);

axis2_bool_t AXIS2_CALL
savan_db_mgr_update(
    savan_db_mgr_t *db_mgr,
    const axutil_env_t *env,
    savan_subscriber_t *subscriber,
    axis2_char_t *sql_stmt_update);

axutil_array_list_t *AXIS2_CALL
savan_db_mgr_retrieve_all(
    savan_db_mgr_t *db_mgr,
    const axutil_env_t *env,
    savan_subscriber_t *subscriber,
    int (*find_func)(void *, int, char **, char **),
    axis2_char_t *sql_stmt_find);

void * AXIS2_CALL
savan_db_mgr_get_dbconn(
    savan_db_mgr_t *db_mgr, 
    const axutil_env_t *env);

/** @} */
#ifdef __cplusplus
}
#endif
#endif /* SAVAN_DB_MGR_H */