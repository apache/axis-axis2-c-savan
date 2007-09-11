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
#include <axis2_conf_ctx.h>
#include <axis2_svc.h>
#include <platforms/axutil_platform_auto_sense.h>

#include <savan_publishing_client.h>
#include <savan_subscriber.h>
#include <savan_util.h>
#include <savan_constants.h>

struct savan_publishing_client_t
{
    axis2_conf_ctx_t *conf_ctx;
    axis2_svc_t *svc;
};

/******************************************************************************/


/******************************************************************************/

AXIS2_EXTERN savan_publishing_client_t * AXIS2_CALL
savan_publishing_client_create(
    const axutil_env_t *env,
    axis2_conf_ctx_t *conf_ctx,
    axis2_svc_t *svc)
{
    savan_publishing_client_t *client = NULL;

    AXIS2_ENV_CHECK(env, NULL);

    client = AXIS2_MALLOC(env->allocator, sizeof(savan_publishing_client_t));

    if (!client)
    {
        AXIS2_ERROR_SET(env->error, AXIS2_ERROR_NO_MEMORY, AXIS2_FAILURE);
        return NULL;
    }

    client->conf_ctx = conf_ctx;
    client->svc = svc;

    return client;
}

/******************************************************************************/

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
    axiom_node_t *payload)
{
    axutil_param_t *param = NULL;
    axutil_hash_index_t *hi = NULL;
    axis2_svc_t *pubs_svc = NULL;
    axutil_hash_t *subs_store = NULL;

    AXIS2_ENV_CHECK(env, AXIS2_FAILURE);

    AXIS2_LOG_DEBUG(env->log, AXIS2_LOG_SI, "[savan] "
        "Start:savan_publishing_client_publish");
   
    pubs_svc = client->svc;
    param = axis2_svc_get_param(pubs_svc, env, "SubscriptionMgrName");
    if(param)
    {
        axis2_svc_t *subs_svc = NULL;
        axis2_conf_ctx_t *conf_ctx = NULL;
        axis2_conf_t *conf = NULL;
        axutil_param_t *subs_store_param = NULL;
        axis2_char_t *subs_svc_name = NULL;

        subs_svc_name = axutil_param_get_value(param, env);
        conf_ctx = client->conf_ctx;
        conf = axis2_conf_ctx_get_conf(conf_ctx, env);
        if(conf)
            subs_svc = axis2_conf_get_svc(conf, env, subs_svc_name);
        if(subs_svc)
        {
            subs_store_param = axis2_svc_get_param(subs_svc, env,
                SAVAN_SUBSCRIBER_LIST);
            if(!subs_store_param)
            {
                AXIS2_LOG_DEBUG(env->log, AXIS2_LOG_SI, 
                    "[SAVAN] No Subscriber found");
                return AXIS2_SUCCESS;/* returning FAILURE will break handler chain */
            }
            subs_store = axutil_param_get_value(subs_store_param, env);
        }
        else
        {
            axis2_char_t *subs_mgr_url = NULL;

            param = axis2_svc_get_param(pubs_svc, env, "SubscriptionMgrURL");
            if(param)
            {
                axis2_svc_client_t* svc_client = NULL;
                axutil_param_t *svc_client_param = NULL;
                axutil_param_t *topic_param = NULL;
                axis2_char_t *topic_url = NULL;

                subs_mgr_url = axutil_param_get_value(param, env);
                topic_param = axis2_svc_get_param(pubs_svc, env, "TopicURL");
                topic_url = axutil_param_get_value(topic_param, env);
                svc_client_param = axis2_svc_get_param(pubs_svc, env, "svc_client");
                if(svc_client_param)
                    svc_client = axutil_param_get_value(svc_client_param, env);
                if(!svc_client)
                {
                    svc_client = 
                        (axis2_svc_client_t *) savan_util_get_svc_client(env);
                    svc_client_param = axutil_param_create(env, "svc_client", 
                        svc_client);
                    axis2_svc_add_param(pubs_svc, env, svc_client_param);
                }
                subs_store = 
                    savan_util_get_subscriber_list_from_remote_subs_mgr(env, 
                        topic_url, subs_mgr_url, svc_client);
            }
        }
    }
    else
    { 
        param = axis2_svc_get_param(pubs_svc, env, SAVAN_SUBSCRIBER_LIST);
        if (param)
        {
            subs_store = (axutil_hash_t*)axutil_param_get_value(param, env);
        }
    }
    if (!subs_store)
    {
        AXIS2_LOG_ERROR(env->log, AXIS2_LOG_SI, 
            "[SAVAN] Subscriber store is null"); 
        return AXIS2_SUCCESS; /* returning FAILURE will break handler chain */
    }

    for (hi = axutil_hash_first(subs_store, env); hi; hi =
        axutil_hash_next(env, hi))
    {
        void *val = NULL;
        savan_subscriber_t * sub = NULL;
        axutil_hash_this(hi, NULL, NULL, &val);
        sub = (savan_subscriber_t *)val;
        if (sub)
        {
            axis2_char_t *id = savan_subscriber_get_id(sub, env);
            AXIS2_LOG_DEBUG(env->log, AXIS2_LOG_SI, "[savan][out handler] "
                "Publishing to %s", id);
            savan_subscriber_publish(sub, env, payload);
        }

        val = NULL;
    }
    AXIS2_LOG_DEBUG(env->log, AXIS2_LOG_SI, "[savan] "
        "End:savan_publishing_client_publish");
    
    return AXIS2_SUCCESS;
}

