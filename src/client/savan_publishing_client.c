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

#include <axiom_node.h>
#include <axiom_element.h>
#include <axiom_soap_body.h>
#include <axis2_options.h>
#include <axutil_array_list.h>
#include <axis2_conf.h>
#include <axis2_svc.h>
#include <axis2_svc_client.h>
#include <axis2_endpoint_ref.h>
#include <platforms/axutil_platform_auto_sense.h>

#include <savan_publishing_client.h>
#include <savan_subscriber.h>
#include <savan_util.h>
#include <savan_constants.h>
#include <savan_storage_mgr.h>

struct savan_publishing_client_t
{
    axis2_conf_t *conf;
    axis2_svc_t *svc;
};

AXIS2_EXTERN savan_publishing_client_t * AXIS2_CALL
savan_publishing_client_create(
    const axutil_env_t *env,
    axis2_conf_t *conf,
    axis2_svc_t *svc)
{
    savan_publishing_client_t *client = NULL;

    client = AXIS2_MALLOC(env->allocator, sizeof(savan_publishing_client_t));

    if (!client)
    {
        AXIS2_ERROR_SET(env->error, AXIS2_ERROR_NO_MEMORY, AXIS2_FAILURE);
        AXIS2_LOG_ERROR(env->log, AXIS2_LOG_SI, 
           "[savan] Memory allocation failed for Savan Publishing Client");
        return NULL;
    }

    client->conf = conf;
    client->svc = svc;

    return client;
}

AXIS2_EXTERN void AXIS2_CALL
savan_publishing_client_free(
    savan_publishing_client_t *client, 
    const axutil_env_t *env)
{
    AXIS2_FREE(env->allocator, client);
}

AXIS2_EXTERN axis2_status_t AXIS2_CALL
savan_publishing_client_publish(
    savan_publishing_client_t *client,
    const axutil_env_t *env,
    axiom_node_t *payload,
    axis2_char_t *filter)
{
    axis2_svc_t *pubs_svc = NULL;
    axutil_array_list_t *subs_store = NULL;
    axis2_conf_t *conf = NULL;
    int i = 0, size = 0;
    savan_storage_mgr_t *storage_mgr = NULL;
    savan_filter_mod_t *filtermod = NULL;

    AXIS2_LOG_TRACE(env->log, AXIS2_LOG_SI, "[savan] Entry:savan_publishing_client_publish");
   
    conf = client->conf;
    pubs_svc = client->svc;

    storage_mgr = savan_util_get_storage_mgr(env, NULL, conf);
    if(storage_mgr)
    {
        subs_store = savan_storage_mgr_retrieve_all_subscribers(storage_mgr, env, filter);
    }

    if (!subs_store)
    {
        AXIS2_LOG_WARNING(env->log, AXIS2_LOG_SI, "[savan] Subscriber store is NULL"); 
        return AXIS2_SUCCESS; /* returning FAILURE will break handler chain */
    }

    size = axutil_array_list_size(subs_store, env);
    for(i = 0; i < size; i++)
    {
        savan_subscriber_t *sub = NULL;
        sub = (savan_subscriber_t *)axutil_array_list_get(subs_store, env, i);
        if (sub)
        {
            axis2_char_t *id = savan_subscriber_get_id(sub, env);
            AXIS2_LOG_DEBUG(env->log, AXIS2_LOG_SI, "[savan] Publishing to:%s", id);

            filtermod = savan_util_get_filter_module(env, client->conf);
            if(!savan_subscriber_publish(sub, env, filtermod, payload))
            {
                axis2_endpoint_ref_t *notifyto = savan_subscriber_get_notify_to(sub, env);
                const axis2_char_t *address = NULL;

                if(notifyto)
                {
                    address = axis2_endpoint_ref_get_address(notifyto, env);
                }

                AXIS2_LOG_ERROR(env->log, AXIS2_LOG_SI, 
                        "Publishing to the Data Sink:%s proviced by subscriber:%s Failed. Check "\
                        "whether the Data Sink url is correct", address, id);
            }
        }
    }

    AXIS2_LOG_TRACE(env->log, AXIS2_LOG_SI, "[savan] Exit:savan_publishing_client_publish");
    
    return AXIS2_SUCCESS;
}


