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
#include <savan_db_mgr.h>

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
    axis2_char_t *topic_url)
{
    axutil_param_t *param = NULL;
    axis2_svc_t *pubs_svc = NULL;
    axutil_array_list_t *subs_store = NULL;
    axis2_conf_t *conf = NULL;
    axis2_module_desc_t *module_desc = NULL;
    int i = 0, size = 0;
    axutil_param_t *topic_param = NULL;
    axutil_qname_t *qname = NULL;

    AXIS2_LOG_TRACE(env->log, AXIS2_LOG_SI, "[savan] Entry:savan_publishing_client_publish");
   
    conf = client->conf;
    pubs_svc = client->svc;

    if(!topic_url)
    {
        topic_param = axis2_svc_get_param(pubs_svc, env, SAVAN_TOPIC_URL);
        if (!topic_param)
        {
            AXIS2_LOG_ERROR(env->log, AXIS2_LOG_SI, "[savan] TopicURL param not available");
            return AXIS2_SUCCESS;
        }

        topic_url = axutil_param_get_value(topic_param, env);
    }

    qname = axutil_qname_create(env, SAVAN_MODULE, NULL, NULL);
    module_desc = axis2_conf_get_module(conf, env, qname);

    param = axis2_module_desc_get_param(module_desc, env, SAVAN_SUBSCRIPTION_MGR_URL);
    axutil_qname_free(qname, env);

    if(param)
    {
        axis2_char_t *subs_mgr_url = NULL;

        axis2_svc_client_t* svc_client = NULL;
        axutil_param_t *svc_client_param = NULL;

        AXIS2_LOG_DEBUG(env->log, AXIS2_LOG_SI, 
            "[savan] Using remote subscription manager to retrieve subscribers for %s", topic_url);
        subs_mgr_url = axutil_param_get_value(param, env);
        svc_client_param = axis2_svc_get_param(pubs_svc, env, SAVAN_SVC_CLIENT);
        if(svc_client_param)
        {
            svc_client = axutil_param_get_value(svc_client_param, env);
        }

        if(!svc_client)
        {
            svc_client = (axis2_svc_client_t *) savan_util_get_svc_client(env);
            svc_client_param = axutil_param_create(env, SAVAN_SVC_CLIENT, svc_client);
            axis2_svc_add_param(pubs_svc, env, svc_client_param);
        }

        subs_store = savan_util_get_subscriber_list_from_remote_subs_mgr(env, topic_url, 
                subs_mgr_url, svc_client, conf);
    }
    else
    {
        axis2_char_t sql_retrieve[256];
        axis2_char_t *topic_name = NULL;

        AXIS2_LOG_DEBUG(env->log, AXIS2_LOG_SI, 
            "[savan] Using local subscription manager to retrieve subscribers for %s", topic_url);

        topic_name = savan_util_get_topic_name_from_topic_url(env, topic_url);
        sprintf(sql_retrieve, "select id, end_to, notify_to, delivery_mode, "\
            "expires, filter, renewed, topic_url from subscriber, topic"\
            " where topic.topic_name=subscriber.topic_name and"\
            " topic.topic_name='%s';", topic_name);

        AXIS2_LOG_DEBUG(env->log, AXIS2_LOG_SI, "[savan] sql_retrieve:%s", sql_retrieve);
        subs_store = savan_db_mgr_retrieve_all(env, savan_util_get_dbname(env, conf),
                                               savan_db_mgr_subs_find_callback, 
                                               sql_retrieve);
    }

    if (!subs_store)
    {
        AXIS2_LOG_ERROR(env->log, AXIS2_LOG_SI, "[savan] Subscriber store is NULL"); 
        return AXIS2_SUCCESS; /* returning FAILURE will break handler chain */
    }

    size = axutil_array_list_size(subs_store, env);
    for(i = 0; i < size; i++)
    {
        savan_subscriber_t *sub = NULL;
        sub = (savan_subscriber_t *)axutil_array_list_get(subs_store, env, i);
        if (sub)
        {
            axis2_char_t *filter_template_path = NULL;
            axis2_char_t *id = savan_subscriber_get_id(sub, env);
            AXIS2_LOG_DEBUG(env->log, AXIS2_LOG_SI, "[savan] Publishing to:%s", id);

            filter_template_path = savan_util_get_module_param(env, conf, 
                    SAVAN_FILTER_TEMPLATE_PATH);
            savan_subscriber_set_filter_template_path(sub, env, filter_template_path);
            if(!savan_subscriber_publish(sub, env, payload))
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


