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
savan_subs_mgr_free(
    savan_subs_mgr_t *subs_mgr,
    const axutil_env_t *env)
{
     return subs_mgr->ops->free(subs_mgr, env);
}

AXIS2_EXTERN axis2_status_t AXIS2_CALL
savan_subs_mgr_insert_subscriber(
    savan_subs_mgr_t *subs_mgr, 
    const axutil_env_t *env,
    savan_subscriber_t *subscriber)
{
    return subs_mgr->ops->insert_subscriber(subs_mgr, env, subscriber);
}

AXIS2_EXTERN axis2_status_t AXIS2_CALL
savan_subs_mgr_update_subscriber(
    savan_subs_mgr_t *subs_mgr, 
    const axutil_env_t *env,
    savan_subscriber_t *subscriber)
{
    return subs_mgr->ops->update_subscriber(subs_mgr, env, subscriber);
}

AXIS2_EXTERN axis2_status_t AXIS2_CALL
savan_subs_mgr_remove_subscriber(
    savan_subs_mgr_t *subs_mgr, 
    const axutil_env_t *env,
    const axis2_char_t *subscriber_id)
{
    return subs_mgr->ops->remove_subscriber(subs_mgr, env, subscriber_id);
}

AXIS2_EXTERN savan_subscriber_t *AXIS2_CALL
savan_subs_mgr_retrieve_subscriber(
    savan_subs_mgr_t *subs_mgr, 
    const axutil_env_t *env,
    const axis2_char_t *subscriber_id)
{
    return subs_mgr->ops->retrieve_subscriber(subs_mgr, env, subscriber_id);
}

AXIS2_EXTERN axutil_array_list_t * AXIS2_CALL
savan_subs_mgr_retrieve_all_subscribers(
    savan_subs_mgr_t *subs_mgr, 
    const axutil_env_t *env,
    const axis2_char_t *filter)
{
    return subs_mgr->ops->retrieve_all_subscribers(subs_mgr, env, filter);
}

AXIS2_EXTERN axis2_status_t AXIS2_CALL
savan_subs_mgr_insert_topic(
    savan_subs_mgr_t *subs_mgr, 
    const axutil_env_t *env,
    const axis2_char_t *topic_name,
    const axis2_char_t *topic_url)
{
    return subs_mgr->ops->insert_topic(subs_mgr, env, topic_name, topic_url);
}

