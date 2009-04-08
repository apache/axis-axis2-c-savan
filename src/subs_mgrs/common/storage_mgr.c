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
 
#include <savan_storage_mgr.h>
#include <savan_constants.h>
#include <savan_error.h>
#include <savan_util.h>
#include <axutil_log.h>
#include <axutil_hash.h>
#include <axutil_property.h>
#include <axutil_uuid_gen.h>
#include <axis2_conf_ctx.h>

AXIS2_EXTERN void AXIS2_CALL
savan_storage_mgr_free(
    savan_storage_mgr_t *storage_mgr,
    const axutil_env_t *env)
{
     return storage_mgr->ops->free(storage_mgr, env);
}

AXIS2_EXTERN axis2_status_t AXIS2_CALL
savan_storage_mgr_insert_subscriber(
    savan_storage_mgr_t *storage_mgr, 
    const axutil_env_t *env,
    savan_subscriber_t *subscriber)
{
    return storage_mgr->ops->insert_subscriber(storage_mgr, env, subscriber);
}

AXIS2_EXTERN axis2_status_t AXIS2_CALL
savan_storage_mgr_update_subscriber(
    savan_storage_mgr_t *storage_mgr, 
    const axutil_env_t *env,
    savan_subscriber_t *subscriber)
{
    return storage_mgr->ops->update_subscriber(storage_mgr, env, subscriber);
}

AXIS2_EXTERN axis2_status_t AXIS2_CALL
savan_storage_mgr_remove_subscriber(
    savan_storage_mgr_t *storage_mgr, 
    const axutil_env_t *env,
    const axis2_char_t *subscriber_id)
{
    return storage_mgr->ops->remove_subscriber(storage_mgr, env, subscriber_id);
}

AXIS2_EXTERN savan_subscriber_t *AXIS2_CALL
savan_storage_mgr_retrieve_subscriber(
    savan_storage_mgr_t *storage_mgr, 
    const axutil_env_t *env,
    const axis2_char_t *subscriber_id)
{
    return storage_mgr->ops->retrieve_subscriber(storage_mgr, env, subscriber_id);
}

AXIS2_EXTERN axutil_array_list_t * AXIS2_CALL
savan_storage_mgr_retrieve_all_subscribers(
    savan_storage_mgr_t *storage_mgr, 
    const axutil_env_t *env,
    const axis2_char_t *filter)
{
    return storage_mgr->ops->retrieve_all_subscribers(storage_mgr, env, filter);
}

AXIS2_EXTERN axis2_status_t AXIS2_CALL
savan_storage_mgr_insert_topic(
    savan_storage_mgr_t *storage_mgr, 
    const axutil_env_t *env,
    const axis2_char_t *topic_name,
    const axis2_char_t *topic_url)
{
    return storage_mgr->ops->insert_topic(storage_mgr, env, topic_name, topic_url);
}

