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
#include <savan_error.h>
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
    axis2_char_t *path = NULL;
    axis2_conf_ctx_t *client_conf_ctx = NULL;
    axis2_svc_t *client_svc = NULL;
    axis2_svc_client_t *svc_client = NULL;

    AXIS2_LOG_TRACE(env->log, AXIS2_LOG_SI, "[savan] Entry:savan_publishing_client_publish");
   
    conf = client->conf;
    pubs_svc = client->svc;

    storage_mgr = savan_util_get_storage_mgr(env, NULL, conf);
    axutil_allocator_switch_to_global_pool(env->allocator);
    if(storage_mgr)
    {
        subs_store = savan_storage_mgr_retrieve_all_subscribers(storage_mgr, env, filter);
    }

    if (!subs_store)
    {
        axutil_allocator_switch_to_local_pool(env->allocator);
        AXIS2_LOG_WARNING(env->log, AXIS2_LOG_SI, "[savan] Subscriber store is NULL"); 
        return AXIS2_SUCCESS; /* returning FAILURE will break handler chain */
    }

    path = AXIS2_GETENV("AXIS2C_HOME");
    svc_client = axis2_svc_client_create(env, path);

    if(!svc_client)
    {
        axutil_allocator_switch_to_local_pool(env->allocator);
        AXIS2_LOG_ERROR (env->log, AXIS2_LOG_SI, 
            "[savan]svc_client creation failed, unable to continue");
        return AXIS2_SUCCESS;
    }

    client_conf_ctx = axis2_svc_client_get_conf_ctx(svc_client, env);
    client_svc = axis2_svc_client_get_svc(svc_client, env);

    size = axutil_array_list_size(subs_store, env);
    for(i = 0; i < size; i++)
    {
        axis2_svc_client_t *svc_client = NULL;
        savan_subscriber_t *sub = NULL;

        sub = (savan_subscriber_t *)axutil_array_list_get(subs_store, env, i);
        if (sub)
        {
            axis2_char_t *id = savan_subscriber_get_id(sub, env);
            AXIS2_LOG_DEBUG(env->log, AXIS2_LOG_SI, "[savan] Publishing to:%s", id);

            svc_client = axis2_svc_client_create_with_conf_ctx_and_svc(env, path, client_conf_ctx, 
                    client_svc);
            filtermod = savan_util_get_filter_module(env, client->conf);
            /* Ideally publishing to each subscriber should happen within a thread for each 
             * subscriber. However until Axis2/C provide a good thread pool to handle
             * such tasks I use this sequential publishing to subscribers.
             */
            if(!savan_publishing_client_publish_to_subscriber(client, env, svc_client, sub, 
                        filtermod, payload))
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
            if(svc_client)
            {
                axis2_svc_client_free(svc_client, env);
            }
        }
    }

    if(svc_client)
    {
        axis2_svc_client_free(svc_client, env);
    }
    axutil_allocator_switch_to_local_pool(env->allocator);

    AXIS2_LOG_TRACE(env->log, AXIS2_LOG_SI, "[savan] Exit:savan_publishing_client_publish");
    
    return AXIS2_SUCCESS;
}

AXIS2_EXTERN axis2_status_t AXIS2_CALL
savan_publishing_client_publish_to_subscriber(
    savan_publishing_client_t *client,
    const axutil_env_t *env,
    axis2_svc_client_t *svc_client,
    savan_subscriber_t *subscriber,
    savan_filter_mod_t *filtermod,
    axiom_node_t *payload)
{
    axis2_options_t *options = NULL;
    axis2_status_t status = AXIS2_SUCCESS;
    axis2_endpoint_ref_t *to = NULL;
    const axis2_char_t *address = NULL;
    axis2_bool_t filter_apply = AXIS2_TRUE;
    axis2_endpoint_ref_t *notifyto = NULL;

    AXIS2_LOG_TRACE(env->log, AXIS2_LOG_SI, "[savan] Entry:savan_publishing_client_publish_to_subscriber");

    options = (axis2_options_t *) axis2_svc_client_get_options(svc_client, env);
    if(!options)
    {
        options = axis2_options_create(env);
        axis2_svc_client_set_options(svc_client, env, options);
    }

    notifyto = savan_subscriber_get_notify_to(subscriber, env);
    if(notifyto)
    {
        address = axis2_endpoint_ref_get_address(notifyto, env);
        if(address)
        {
            AXIS2_LOG_DEBUG(env->log, AXIS2_LOG_SI, "[savan] Publishing to:%s", address);
            to = axis2_endpoint_ref_create(env, address);
            axis2_options_set_to(options, env, to);
        }
    }
    axis2_options_set_xml_parser_reset(options, env, AXIS2_FALSE);


    /* If this is a filtering request and we cannot find any filter module to filter then error */
    if(savan_subscriber_get_filter(subscriber, env) && !filtermod)
    {
        AXIS2_HANDLE_ERROR(env, SAVAN_ERROR_FILTER_MODULE_COULD_NOT_BE_RETRIEVED, AXIS2_FAILURE);
        return AXIS2_FAILURE;
    }

#ifdef SAVAN_FILTERING
    /* If this is a filtering request and filter module is defined then filter the request.
     */
    if(filtermod && savan_subscriber_get_filter(subscriber, env))
    {
	    /* Apply the filter, and check whether it evaluates to success */
        filter_apply = savan_filter_mod_apply(filtermod ,env, subscriber, payload);
        if(!filter_apply)
        {
            status = axutil_error_get_status_code(env->error);
            if(AXIS2_SUCCESS != status)
            {
                axiom_node_detach(payload, env);
                return status;
            }
        }
    }
#endif

    if(filter_apply)
    {
        axis2_svc_client_fire_and_forget(svc_client, env, payload);
    }

    axiom_node_detach(payload, env); /*insert this to prevent payload corruption in subsequent 
                                       "publish" calls with some payload.*/

    AXIS2_LOG_TRACE(env->log, AXIS2_LOG_SI, "[savan] Exit:savan_publishing_client_publish_to_subscriber");

    return status;
}


