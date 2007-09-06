/*
 * Copyright 2004,2005 The Apache Software Foundation.
 *
 * Licensed under the Apache License, Version 2.0 (the "License")
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

#include <axis2_msg_info_headers.h>
#include <axis2_options.h>
#include <axis2_svc_client.h>
#include <axis2_endpoint_ref.h>
#include <platforms/axutil_platform_auto_sense.h>
#include <axiom_soap.h>

#include <savan_util.h>
#include <savan_error.h>
/*#include <libxslt/xsltutils.h>*/

/******************************************************************************/

/*axis2_status_t
savan_util_update_filter_template(
    xmlNodeSetPtr nodes,
    const xmlChar* value);*/

axiom_node_t*
savan_util_create_fault_msg(axis2_char_t *code,
    axis2_char_t* subcode, axis2_char_t *reason,
    axis2_char_t* details,
    const axutil_env_t *env);

static axis2_status_t
add_subscriber_to_remote_subs_mgr(
    const axutil_env_t *env,
    savan_subscriber_t *subscriber,
    axis2_char_t *subs_mgr_url);

static axis2_status_t
remove_subscriber_from_remote_subs_mgr(
    const axutil_env_t *env,
    savan_subscriber_t *subscriber,
    axis2_char_t *subs_mgr_url);

static axiom_node_t *
build_add_subscriber_om_payload(
    const axutil_env_t *env,
    savan_subscriber_t *subscriber);

static axiom_node_t *
build_remove_subscriber_om_payload(
    const axutil_env_t *env,
    savan_subscriber_t *subscriber);

static axiom_node_t *
build_subscribers_request_om_payload(
    const axutil_env_t *env,
    axis2_char_t *topic);

static axiom_node_t *
build_topics_request_om_payload(
    const axutil_env_t *env);

static axutil_hash_t *
process_subscriber_list_node(
    const axutil_env_t *env,
    axiom_node_t *subs_list_node);

static axutil_array_list_t *
process_topic_list_node(
    const axutil_env_t *env,
    axiom_node_t *subs_list_node);

/*axis2_status_t AXIS2_CALL
savan_util_set_filter_template_for_subscriber(
    savan_subscriber_t *subscriber,
    savan_sub_processor_t *sub_processor,
    const axutil_env_t *env)
{
    AXIS2_ENV_CHECK(env, NULL);
    xsltStylesheetPtr xslt_template_xslt = NULL;
    xmlDocPtr xslt_template_xml = NULL;

	if(savan_subscriber_get_filter(subscriber, env) == NULL)
	{
		return AXIS2_SUCCESS;
	}

    xslt_template_xml = xmlParseFile("../modules/savan/template.xsl");
    xmlChar* xpathExpr = (xmlChar*)"//@select";
    xmlChar* value = (xmlChar*)savan_subscriber_get_filter(subscriber,env);
    xmlXPathContextPtr xpathCtx = xmlXPathNewContext(xslt_template_xml);
    xmlXPathObjectPtr xpathObj = xmlXPathEvalExpression(xpathExpr, xpathCtx);
    savan_util_update_filter_template(xpathObj->nodesetval, value);

    xslt_template_xslt = xsltParseStylesheetDoc(xslt_template_xml);
    savan_subscriber_set_filter_template(subscriber, env, xslt_template_xslt);

	xmlXPathFreeObject(xpathObj);
	xmlXPathFreeContext(xpathCtx);

    return AXIS2_SUCCESS;
}*/

/*axiom_node_t * AXIS2_CALL
savan_util_apply_filter(
    savan_subscriber_t *subscriber,
    const axutil_env_t *env,
    axiom_node_t *payload)
{
    xmlChar *buffer = NULL;
    int size = 0;
    axis2_char_t *payload_string = NULL;
    xmlDocPtr payload_doc = NULL;
    xsltStylesheetPtr xslt_template_filter = NULL;

	if(savan_subscriber_get_filter(subscriber, env) == NULL)
	{
		return payload;
	}

    payload_string = axiom_node_to_string(payload, env);

    payload_doc = (xmlDocPtr)xmlParseDoc((xmlChar*)payload_string);

    xslt_template_filter = 
		(xsltStylesheetPtr)savan_subscriber_get_filter_template(subscriber,
        env);

    xmlDocPtr result_doc = (xmlDocPtr)xsltApplyStylesheet(xslt_template_filter,
        payload_doc, NULL);

    xmlDocDumpMemory(result_doc, &buffer, &size);

    axiom_xml_reader_t *reader = axiom_xml_reader_create_for_memory(env, 
		(char*)buffer,axutil_strlen((char*)buffer), 
		NULL, AXIS2_XML_PARSER_TYPE_BUFFER);
    axiom_stax_builder_t *om_builder = axiom_stax_builder_create(env, reader);
    axiom_document_t *document = axiom_stax_builder_get_document(om_builder, 
		env);
    axiom_node_t *node = axiom_document_build_all(document, env);

	if(node == NULL)
	{
		node = savan_util_create_fault_msg("CODE", "FilteringRequestedUnavailabe", 
			"Requested Filter dialect is not supported", "DETAILS", env);
	}

    axiom_stax_builder_free_self(om_builder, env);
	axiom_node_free_tree(payload, env);
    free(payload_string);
	xmlFreeDoc(result_doc);

    return node;
}

axis2_status_t 
savan_util_update_filter_template(
    xmlNodeSetPtr nodes,
    const xmlChar* value)
{
    int size;
    int i;
    size = (nodes) ? nodes->nodeNr : 0;
    for(i = size - 1; i >= 0; i--) 
	{
    	xmlNodeSetContent(nodes->nodeTab[i], value);
    	if (nodes->nodeTab[i]->type != XML_NAMESPACE_DECL)
        	nodes->nodeTab[i] = NULL;
    }
    return AXIS2_SUCCESS;
}*/

axiom_node_t*
savan_util_create_fault_msg(axis2_char_t *code,
    axis2_char_t* subcode, axis2_char_t *reason,
	axis2_char_t* details,
	const axutil_env_t *env)
{
    axiom_node_t *fault_node = NULL;
    axiom_element_t *fault_ele = NULL;
    axiom_node_t *fault_reason_node = NULL;
    axiom_element_t *fault_reason_ele = NULL;
    axiom_node_t *fault_code_node = NULL;
    axiom_element_t *fault_code_ele = NULL;

	/*
    axiom_node_t *fault_subcode_node = NULL;
    axiom_element_t *fault_subcode_ele = NULL;
    axiom_node_t *fault_details_node = NULL;
    axiom_element_t *fault_details_ele = NULL;
	*/

    fault_ele = axiom_element_create(env, NULL, "Fault", NULL, &fault_node);
	fault_code_ele = axiom_element_create(env, fault_node, "faultcode", 
		NULL, &fault_code_node);
    axiom_element_set_text(fault_code_ele, env, subcode, fault_code_node);
	fault_reason_ele = axiom_element_create(env, fault_node, "faultstring", 
		NULL, &fault_reason_node);
    axiom_element_set_text(fault_reason_ele, env, reason, fault_reason_node);
    return fault_node;
}

savan_message_types_t AXIS2_CALL
savan_util_get_message_type(
    axis2_msg_ctx_t *msg_ctx,
    const axutil_env_t *env)
{
    const axis2_char_t *action = NULL;
    axis2_msg_info_headers_t *info_header = NULL;

    info_header =  axis2_msg_ctx_get_msg_info_headers(msg_ctx, env);
    if (!info_header)
    {
        AXIS2_LOG_ERROR(env->log, AXIS2_LOG_SI, 
            "[SAVAN] Cannot extract message info headers"); 
        return SAVAN_MSG_TYPE_UNKNOWN;
    }
    
    action = axis2_msg_info_headers_get_action(info_header, env);
    if( ! action)
    {
        AXIS2_LOG_ERROR(env->log, AXIS2_LOG_SI, 
            "[SAVAN] Cannot extract soap action"); 
        AXIS2_ERROR_SET(env->error, SAVAN_ERROR_SOAP_ACTION_NULL, AXIS2_FAILURE);
        return SAVAN_MSG_TYPE_UNKNOWN;
    }
    
    if (axutil_strcmp(action, SAVAN_ACTIONS_SUB) == 0)
        return SAVAN_MSG_TYPE_SUB;
    else if (axutil_strcmp(action, SAVAN_ACTIONS_SUB_RESPONSE) == 0)
        return SAVAN_MSG_TYPE_SUB_RESPONSE;
    else if (axutil_strcmp(action, SAVAN_ACTIONS_UNSUB) == 0)
        return SAVAN_MSG_TYPE_UNSUB;
    else if (axutil_strcmp(action, SAVAN_ACTIONS_UNSUB_RESPONSE) == 0)
        return SAVAN_MSG_TYPE_UNSUB_RESPONSE;
    else if (axutil_strcmp(action, SAVAN_ACTIONS_GET_STATUS) == 0)
        return SAVAN_MSG_TYPE_GET_STATUS;
    else if (axutil_strcmp(action, SAVAN_ACTIONS_GET_STATUS_RESPONSE) == 0)
        return SAVAN_MSG_TYPE_GET_STATUS_RESPONSE;
    else if (axutil_strcmp(action, SAVAN_ACTIONS_RENEW) == 0)
        return SAVAN_MSG_TYPE_RENEW;
    else if (axutil_strcmp(action, SAVAN_ACTIONS_RENEW_RESPONSE) == 0)
        return SAVAN_MSG_TYPE_RENEW_RESPONSE;
    
    return SAVAN_MSG_TYPE_UNKNOWN;
}

/******************************************************************************/

axis2_char_t * AXIS2_CALL
savan_util_get_subscription_id_from_msg(
    const axutil_env_t *env,
    axis2_msg_ctx_t *msg_ctx)
{
    axis2_char_t *sub_id = NULL;
    axiom_soap_envelope_t *envelope = NULL;
    axiom_soap_header_t *header = NULL;
    axutil_qname_t *qname = NULL;
    axiom_node_t *header_node = NULL;
    axiom_node_t *id_node = NULL;
    axiom_element_t *header_elem = NULL;
    axiom_element_t *id_elem = NULL;
    
    AXIS2_ENV_CHECK(env, NULL);
    
    AXIS2_LOG_DEBUG(env->log, AXIS2_LOG_SI, 
        "[SAVAN] Start:savan_util_get_subscription_id_from_msg");
    
    /* Get soap envelop and extract the subscription id */

    envelope =  axis2_msg_ctx_get_soap_envelope(msg_ctx, env);
    if (!envelope)
    {
        AXIS2_LOG_ERROR(env->log, AXIS2_LOG_SI, 
            "[SAVAN] Failed to extract the soap envelop");
        return NULL;
    }
    
    header = axiom_soap_envelope_get_header(envelope, env);
    if (!header)
    {
        AXIS2_LOG_ERROR(env->log, AXIS2_LOG_SI, 
            "[SAVAN] Failed to extract the soap header"); 
        return NULL;
    }
    
    /* Get header element from header node */
    header_node = axiom_soap_header_get_base_node(header, env);
    header_elem = (axiom_element_t*)axiom_node_get_data_element(header_node, env);
    
    /* Get Identifier element from header */
    qname = axutil_qname_create(env, ELEM_NAME_ID, EVENTING_NAMESPACE, NULL);
    id_elem = axiom_element_get_first_child_with_qname(header_elem, env, qname,
        header_node, &id_node);
    axutil_qname_free(qname, env);
    
    /* Now read the id */
    sub_id = axiom_element_get_text(id_elem, env, id_node);
    
    AXIS2_LOG_DEBUG(env->log, AXIS2_LOG_SI, 
        "[SAVAN] End:savan_util_get_subscription_id_from_msg");
    return sub_id;    
}

/******************************************************************************/

savan_subscriber_t * AXIS2_CALL
savan_util_get_subscriber_from_msg(
        const axutil_env_t *env,
        axis2_msg_ctx_t *msg_ctx,
        axis2_char_t *sub_id)
{
    axutil_hash_t *store = NULL;
    savan_subscriber_t *subscriber = NULL;

    AXIS2_ENV_CHECK(env, NULL);

    AXIS2_LOG_DEBUG(env->log, AXIS2_LOG_SI, 
        "[SAVAN] Start:savan_util_get_subscriber_from_msg");

    /* Extract subscription id from msg if not already given */
    if (!sub_id)
    {
        sub_id = savan_util_get_subscription_id_from_msg(env, msg_ctx);
    }

    store = savan_util_get_subscriber_store(env, msg_ctx);
    if (!store)
    {
        AXIS2_LOG_ERROR(env->log, AXIS2_LOG_SI, 
            "[SAVAN] Subscriber store is null"); 
        return NULL;
    }
    subscriber = axutil_hash_get(store, sub_id, AXIS2_HASH_KEY_STRING);
    AXIS2_LOG_DEBUG(env->log, AXIS2_LOG_SI, 
        "[SAVAN] End:savan_util_get_subscriber_from_msg");
    
    return subscriber;
}

/******************************************************************************/

axutil_hash_t * AXIS2_CALL
savan_util_get_subscriber_store(
    const axutil_env_t *env,
    axis2_msg_ctx_t *msg_ctx)
{
    axis2_svc_t *pubs_svc = NULL;
    axutil_param_t *param = NULL;
    axutil_hash_t *subs_store = NULL;
    axis2_char_t *subs_svc_name = NULL;
    axis2_char_t *topic = NULL;
    axis2_endpoint_ref_t *topic_epr = NULL;
    axis2_char_t *topic_url = NULL;

    AXIS2_ENV_CHECK(env, NULL);

    AXIS2_LOG_DEBUG(env->log, AXIS2_LOG_SI, 
        "[SAVAN] Start:savan_util_get_subscriber_store");

    topic_epr = axis2_msg_ctx_get_to(msg_ctx, env);
    topic_url = (axis2_char_t *) axis2_endpoint_ref_get_address(topic_epr, 
        env);
    topic = savan_util_get_topic_name_from_topic_url(env, topic_url);
    pubs_svc = axis2_msg_ctx_get_svc(msg_ctx, env);
    if (!pubs_svc)
    {
        AXIS2_LOG_ERROR(env->log, AXIS2_LOG_SI, 
            "[SAVAN] Failed to extract the %s publisher service", topic); 
        return NULL;
    }
    param = axis2_svc_get_param(pubs_svc, env, "SubscriptionMgrName");
    if(param)
    {
        axis2_svc_t *subs_svc = NULL;
        axis2_conf_ctx_t *conf_ctx = NULL;
        axis2_conf_t *conf = NULL;
        axutil_param_t *subs_store_param = NULL;

        subs_svc_name = axutil_param_get_value(param, env);
        conf_ctx = axis2_msg_ctx_get_conf_ctx(msg_ctx, env);
        conf = axis2_conf_ctx_get_conf(conf_ctx, env);
        if(conf)
            subs_svc = axis2_conf_get_svc(conf, env, subs_svc_name);
        if(subs_svc)
        {
            subs_store_param = axis2_svc_get_param(subs_svc, env,
                SAVAN_SUBSCRIBER_LIST);
            if(!subs_store_param)
            {
                AXIS2_LOG_DEBUG(env->log, AXIS2_LOG_SI, "[SAVAN] No Topic found");
                return NULL;
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

                subs_mgr_url = axutil_param_get_value(param, env);
                topic_epr = axis2_msg_ctx_get_to(msg_ctx, env);
                topic_url = (axis2_char_t *) axis2_endpoint_ref_get_address(topic_epr, 
                    env);
                svc_client_param = axis2_svc_get_param(pubs_svc, env, "svc_client");
                if(svc_client_param)
                    svc_client = axutil_param_get_value(svc_client_param, env);
                if(!svc_client)
                {
                    svc_client = (axis2_svc_client_t *) savan_util_get_svc_client(env);
                    svc_client_param = axutil_param_create(env, "svc_client", svc_client);
                    axis2_svc_add_param(pubs_svc, env, svc_client_param);
                }
                subs_store = savan_util_get_subscriber_list_from_remote_subs_mgr(env, 
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
    if(!subs_store)
    {
        AXIS2_LOG_DEBUG(env->log, AXIS2_LOG_SI,
            "[SAVAN] No subscribers for topic %s found", topic);
        return NULL;
    }
    AXIS2_LOG_DEBUG(env->log, AXIS2_LOG_SI, 
        "[SAVAN] End:savan_util_get_subscriber_store");
    return subs_store;
}

axis2_status_t AXIS2_CALL
savan_util_add_subscriber(
    const axutil_env_t *env,
    axis2_msg_ctx_t *msg_ctx,
    savan_subscriber_t *subscriber)
{
    axis2_svc_t *pubs_svc = NULL;
    axutil_param_t *param = NULL;
    axis2_char_t *subs_svc_name = NULL;
    axis2_endpoint_ref_t *topic_epr = NULL;
    axis2_char_t *topic_url = NULL;
    axis2_char_t *topic = NULL;
    axutil_hash_t *subs_store = NULL;

    AXIS2_LOG_DEBUG(env->log, AXIS2_LOG_SI, 
        "[SAVAN] Start:savan_util_add_subscriber"); 
    topic_epr = axis2_msg_ctx_get_to(msg_ctx, env);
    topic_url = (axis2_char_t *) axis2_endpoint_ref_get_address(topic_epr, 
        env);
    topic = savan_util_get_topic_name_from_topic_url(env, topic_url);
    pubs_svc = axis2_msg_ctx_get_svc(msg_ctx, env);
    if (!pubs_svc)
    {
        AXIS2_LOG_ERROR(env->log, AXIS2_LOG_SI, 
            "[SAVAN] Failed to extract the %s publisher service", topic); 
        return AXIS2_FAILURE;
    }
    param = axis2_svc_get_param(pubs_svc, env, "SubscriptionMgrName");
    if(param)
    {
        axis2_svc_t *subs_svc = NULL;
        axutil_param_t *subs_store_param = NULL;
        axis2_conf_ctx_t *conf_ctx = NULL;
        axis2_conf_t *conf = NULL;
        
        subs_svc_name = axutil_param_get_value(param, env);
        conf_ctx = axis2_msg_ctx_get_conf_ctx(msg_ctx, env);
        conf = axis2_conf_ctx_get_conf(conf_ctx, env);
        if(conf)
            subs_svc = axis2_conf_get_svc(conf, env, subs_svc_name);
        if(subs_svc)
        {
            axis2_char_t *subs_id = NULL;
            subs_id = savan_subscriber_get_id(subscriber, env);
            subs_store_param = axis2_svc_get_param(subs_svc, env, 
                SAVAN_SUBSCRIBER_LIST);
            if(!subs_store_param)
            {
                savan_util_set_store(subs_svc, env, SAVAN_SUBSCRIBER_LIST);
                subs_store_param = axis2_svc_get_param(subs_svc, env, 
                    SAVAN_SUBSCRIBER_LIST);
            }
            subs_store = axutil_param_get_value(subs_store_param, env);
            if(subs_store)
            {
                axutil_hash_set(subs_store, subs_id, 
                    AXIS2_HASH_KEY_STRING, subscriber);
            }
        }
        else
        {
            axis2_char_t *subs_mgr_url = NULL;
            param = axis2_svc_get_param(pubs_svc, env, "SubscriptionMgrURL");
            subs_mgr_url = axutil_param_get_value(param, env);
            add_subscriber_to_remote_subs_mgr(env, subscriber, subs_mgr_url);
        }
    }
    else
    {
        param = axis2_svc_get_param(pubs_svc, env, SAVAN_SUBSCRIBER_LIST);
        if (!param)
        {
            /* Store not found. Create and set it as a param */
            savan_util_set_store(pubs_svc, env, SAVAN_SUBSCRIBER_LIST);
            param = axis2_svc_get_param(pubs_svc, env, SAVAN_SUBSCRIBER_LIST);
        }
        subs_store = (axutil_hash_t*)axutil_param_get_value(param, env);
        axutil_hash_set(subs_store, savan_subscriber_get_id(subscriber, env), 
            AXIS2_HASH_KEY_STRING, subscriber);
    } 
    AXIS2_LOG_DEBUG(env->log, AXIS2_LOG_SI, 
        "[SAVAN] End:savan_util_add_subscriber"); 
    return AXIS2_SUCCESS;
}

axis2_status_t AXIS2_CALL
savan_util_remove_subscriber(
    const axutil_env_t *env,
    axis2_msg_ctx_t *msg_ctx,
    savan_subscriber_t *subscriber)
{
    axis2_svc_t *pubs_svc = NULL;
    axutil_param_t *param = NULL;
    axutil_hash_t *subs_store = NULL;
    axis2_char_t *subs_svc_name = NULL;
    axis2_char_t *topic_url = NULL;
    axis2_char_t *topic = NULL;
    axis2_endpoint_ref_t *topic_epr = NULL;

    AXIS2_ENV_CHECK(env, AXIS2_FAILURE);

    AXIS2_LOG_DEBUG(env->log, AXIS2_LOG_SI, 
        "[SAVAN] Start:savan_util_remove_subscriber");

    topic_epr = axis2_msg_ctx_get_to(msg_ctx, env);
    topic_url = (axis2_char_t *) axis2_endpoint_ref_get_address(topic_epr, 
        env);
    topic = savan_util_get_topic_name_from_topic_url(env, topic_url);
    pubs_svc = axis2_msg_ctx_get_svc(msg_ctx, env);
    if (!pubs_svc)
    {
        AXIS2_LOG_ERROR(env->log, AXIS2_LOG_SI, 
            "[SAVAN] Failed to extract the %s publisher service", topic); 
        return AXIS2_FAILURE;
    }
    param = axis2_svc_get_param(pubs_svc, env, "SubscriptionMgrName");
    if(param)
    {
        axis2_svc_t *subs_svc = NULL;
        axis2_conf_ctx_t *conf_ctx = NULL;
        axis2_conf_t *conf = NULL;
        axutil_param_t *subs_store_param = NULL;
        
        subs_svc_name = axutil_param_get_value(param, env);
        conf_ctx = axis2_msg_ctx_get_conf_ctx(msg_ctx, env);
        conf = axis2_conf_ctx_get_conf(conf_ctx, env);
        if(conf)
            subs_svc = axis2_conf_get_svc(conf, env, subs_svc_name);
        if(subs_svc)
        {
            axis2_char_t *subs_id = NULL;
            subs_id = savan_subscriber_get_id(subscriber, env);

            subs_store_param = axis2_svc_get_param(subs_svc, env, 
                SAVAN_SUBSCRIBER_LIST);
            if (!subs_store_param)
            {
                AXIS2_LOG_DEBUG(env->log, AXIS2_LOG_SI, 
                    "No Subscriber list found");
                return AXIS2_SUCCESS;
            }
            subs_store = axutil_param_get_value(subs_store_param, env);
            /* Setting NULL as value will remove the entry */
            if(subs_store)
                axutil_hash_set(subs_store, subs_id, AXIS2_HASH_KEY_STRING, 
                    NULL);
        }
        else
        {
            axis2_char_t *subs_mgr_url = NULL;
            param = axis2_svc_get_param(pubs_svc, env, "SubscriptionMgrURL");
            subs_mgr_url = axutil_param_get_value(param, env);
            remove_subscriber_from_remote_subs_mgr(env, subscriber, subs_mgr_url);
        }
    }
    else
    {
        axis2_char_t *subs_id = NULL;

        subs_id = savan_subscriber_get_id(subscriber, env);
        /* Extract the store from the svc and remove the given subscriber */
        param = axis2_svc_get_param(pubs_svc, env, SAVAN_SUBSCRIBER_LIST);
        if (param)
        {
            subs_store = (axutil_hash_t*)axutil_param_get_value(param, env);
        }
        if (!subs_store)
        {
            AXIS2_LOG_DEBUG(env->log, AXIS2_LOG_SI, 
                "[SAVAN] Subscriber store is NULL"); 
            return AXIS2_SUCCESS;
        }
        /* Setting NULL as value will remove the entry */
        axutil_hash_set(subs_store, subs_id, AXIS2_HASH_KEY_STRING, NULL);
    }
    AXIS2_LOG_DEBUG(env->log, AXIS2_LOG_SI, 
        "[SAVAN] End:savan_util_remove_subscriber");
    return AXIS2_SUCCESS;
}

axis2_status_t AXIS2_CALL 
savan_util_set_store(
    axis2_svc_t *svc,
    const axutil_env_t *env,
    axis2_char_t *store_name)
{
    axutil_hash_t *store = NULL;
    axutil_param_t *param = NULL;
    
    AXIS2_ENV_CHECK(env, AXIS2_FAILURE);
    
    AXIS2_LOG_DEBUG(env->log, AXIS2_LOG_SI, "[SAVAN][sub processor] "
        "Start:set_sub_store");
    
    /* Create a hash map */
    store = axutil_hash_make(env);
    if (!store)
    {
        /* TODO : error reporting */
        AXIS2_LOG_ERROR(env->log, AXIS2_LOG_SI, "[SAVAN][sub processor] "
        "Could not create subscriber store");
        return AXIS2_FAILURE;
    }
    
    /* Add the hash map as a parameter to the given service */
    param = axutil_param_create(env, store_name, (void*)store);
    if (!param)
    {
        /* TODO : error reporting */
        AXIS2_LOG_ERROR(env->log, AXIS2_LOG_SI, "[SAVAN][sub processor] "
        "Could not create subscriber store param");
        return AXIS2_FAILURE;
    }
    
    axis2_svc_add_param(svc, env, param);
    AXIS2_LOG_DEBUG(env->log, AXIS2_LOG_SI, "[SAVAN] End:set_sub_store");
    
    return AXIS2_SUCCESS;       
}

static axis2_status_t
add_subscriber_to_remote_subs_mgr(
    const axutil_env_t *env,
    savan_subscriber_t *subscriber,
    axis2_char_t *subs_mgr_url)
{
    const axis2_char_t *address = NULL;
    axis2_endpoint_ref_t* endpoint_ref = NULL;
    axis2_options_t *options = NULL;
    axis2_svc_client_t* svc_client = NULL;
    axiom_node_t *payload = NULL;

    svc_client = (axis2_svc_client_t *) savan_util_get_svc_client(env);
    options = (axis2_options_t *) axis2_svc_client_get_options(svc_client, env);
    address = subs_mgr_url;
    endpoint_ref = axis2_endpoint_ref_create(env, address);
    axis2_options_set_to(options, env, endpoint_ref);
    axis2_options_set_action(options, env,
        "http://ws.apache.org/axis2/c/subscription/add_subscriber");

    payload = build_add_subscriber_om_payload(env, subscriber);
    /* Send request */
    axis2_svc_client_send_robust(svc_client, env, payload);
    if(svc_client)
        axis2_svc_client_free(svc_client, env);

    return AXIS2_SUCCESS;
}

static axis2_status_t
remove_subscriber_from_remote_subs_mgr(
    const axutil_env_t *env,
    savan_subscriber_t *subscriber,
    axis2_char_t *subs_mgr_url)
{
    const axis2_char_t *address = NULL;
    axis2_endpoint_ref_t* endpoint_ref = NULL;
    axis2_options_t *options = NULL;
    axis2_svc_client_t* svc_client = NULL;
    axiom_node_t *payload = NULL;

    svc_client = (axis2_svc_client_t *) savan_util_get_svc_client(env);
    options = (axis2_options_t *) axis2_svc_client_get_options(svc_client, env);
    address = subs_mgr_url;
    endpoint_ref = axis2_endpoint_ref_create(env, address);
    axis2_options_set_to(options, env, endpoint_ref);
    axis2_options_set_action(options, env,
        "http://ws.apache.org/axis2/c/subscription/remove_subscriber");

    payload = build_remove_subscriber_om_payload(env, subscriber);
    /* Send request */
    axis2_svc_client_send_robust(svc_client, env, payload);
    if(svc_client)
        axis2_svc_client_free(svc_client, env);

    return AXIS2_SUCCESS;
}

axutil_hash_t *AXIS2_CALL
savan_util_get_subscriber_list_from_remote_subs_mgr(
    const axutil_env_t *env,
    axis2_char_t *topic,
    axis2_char_t *subs_mgr_url,
    void *s_client)
{
    axis2_endpoint_ref_t* endpoint_ref = NULL;
    axis2_options_t *options = NULL;
    axis2_svc_client_t* svc_client = NULL;
    axiom_node_t *payload = NULL;
    axiom_node_t *ret_node = NULL;
    axutil_hash_t *subscriber_list = NULL;

    AXIS2_LOG_DEBUG(env->log, AXIS2_LOG_SI, 
        "[SAVAN] Start:savan_util_get_subscriber_list_from_remote_subs_mgr");
    svc_client = (axis2_svc_client_t *) s_client;
    options = (axis2_options_t *)axis2_svc_client_get_options(svc_client, 
        env);
    endpoint_ref = axis2_endpoint_ref_create(env, subs_mgr_url);
    axis2_options_set_to(options, env, endpoint_ref);
    
    payload = build_subscribers_request_om_payload(env, topic);
    ret_node = axis2_svc_client_send_receive(svc_client, env, payload);
    if (ret_node)
    {
        subscriber_list = process_subscriber_list_node(env, ret_node);
    }
    else
    {
        AXIS2_LOG_ERROR(env->log, AXIS2_LOG_SI, 
            "[SAVAN] Stub invoke FAILED: Error code:"
            " %d :: %s", env->error->error_number,
            AXIS2_ERROR_GET_MESSAGE(env->error));
    }
    AXIS2_LOG_DEBUG(env->log, AXIS2_LOG_SI, 
        "[SAVAN] End:savan_util_get_subscriber_list_from_remote_subs_mgr");
    return subscriber_list;
}

static axiom_node_t *
build_subscribers_request_om_payload(
    const axutil_env_t *env,
    axis2_char_t *topic)
{
    axiom_node_t *om_node = NULL;
    axiom_element_t* om_ele = NULL;
    axiom_namespace_t *ns1 = NULL;
    axis2_char_t *om_str = NULL;

    ns1 = axiom_namespace_create (env, SAVAN_NAMESPACE, SAVAN_NS_PREFIX);
    om_ele = axiom_element_create(env, NULL, ELEM_NAME_GET_SUBSCRIBER_LIST, ns1, &om_node);
    om_str = axiom_node_to_string(om_node, env);
    if (om_str)
    {
        AXIS2_LOG_DEBUG(env->log, AXIS2_LOG_SI, "Sending OM : %s", om_str);
        AXIS2_FREE(env->allocator, om_str);
        om_str =  NULL;
    }
    return om_node;
}

static axutil_hash_t *
process_subscriber_list_node(
    const axutil_env_t *env,
    axiom_node_t *subs_list_node)
{
    axiom_element_t *subs_list_element = NULL;
    axiom_children_qname_iterator_t *subs_iter = NULL;
    axutil_qname_t *qname = NULL;
    axiom_node_t *topic_node = NULL;
    axiom_element_t *topic_elem = NULL;
    axis2_char_t *topic_url = NULL;
    axutil_hash_t *subscriber_list = NULL;

    AXIS2_LOG_DEBUG(env->log, AXIS2_LOG_SI, 
        "[SAVAN] Start:process_subscriber_list_node");
    subs_list_element = axiom_node_get_data_element(subs_list_node, env); 
         
    /* Topic */
    qname = axutil_qname_create(env, ELEM_NAME_TOPIC, SAVAN_NAMESPACE, NULL);
    topic_elem = axiom_element_get_first_child_with_qname(subs_list_element, 
        env, qname, subs_list_node, &topic_node);
    axutil_qname_free(qname, env);
    topic_url = axiom_element_get_text(topic_elem, env, topic_node);
   
    /* Get Subscriber elements from subscriber list */
    qname = axutil_qname_create(env, ELEM_NAME_SUBSCRIBE, EVENTING_NAMESPACE, 
        NULL);
    subs_iter = axiom_element_get_children_with_qname(subs_list_element, env,
        qname, subs_list_node);
    axutil_qname_free(qname, env);
    if(!subs_iter)
    {
        AXIS2_LOG_DEBUG(env->log, AXIS2_LOG_SI, 
            "[SAVAN] Subscribers list is empty");
        return NULL;
    }
    if(axiom_children_qname_iterator_has_next(subs_iter, env))
    {
        subscriber_list = axutil_hash_make(env);
    }

    while(axiom_children_qname_iterator_has_next(subs_iter, env))
    {
        savan_subscriber_t *subscriber = NULL;
        axiom_node_t *sub_node = NULL;
        axiom_node_t *id_node = NULL;
        axiom_node_t *endto_node = NULL;
        axiom_node_t *delivery_node = NULL;
        axiom_node_t *notify_node = NULL;
        axiom_node_t *filter_node = NULL;
        axiom_node_t *expires_node = NULL;

        axiom_element_t *sub_elem = NULL;
        axiom_element_t *id_elem = NULL;
        axiom_element_t *endto_elem = NULL;
        axiom_element_t *delivery_elem = NULL;
        axiom_element_t *notify_elem = NULL;
        axiom_element_t *expires_elem = NULL;
        axiom_element_t *filter_elem = NULL;

        axis2_char_t *id = NULL;
        axis2_char_t *endto = NULL;
        axis2_char_t *notify = NULL;
        axis2_char_t *expires = NULL;
        axis2_char_t *filter = NULL;

        axis2_endpoint_ref_t *endto_epr = NULL;
        axis2_endpoint_ref_t *notify_epr = NULL;
     
        sub_node = axiom_children_qname_iterator_next(subs_iter, env);
        if(sub_node)
        {
            subscriber = savan_subscriber_create(env);
            if (!subscriber)
            {
                AXIS2_LOG_ERROR(env->log, AXIS2_LOG_SI, "[SAVAN] Failed to create a"
                    "subscriber instance");
                return NULL;
            }
            /* Now read each sub element of Subscribe element */

            /* Topic */
            savan_subscriber_set_topic(subscriber, env, topic_url);

            /* Id */
            qname = axutil_qname_create(env, ELEM_NAME_ID, SAVAN_NAMESPACE, NULL);
            id_elem = axiom_element_get_first_child_with_qname(sub_elem, env, qname,
                sub_node, &id_node);
            axutil_qname_free(qname, env);
            id = axiom_element_get_text(id_elem, env, id_node);
            savan_subscriber_set_id(subscriber, env, id);

            /* EndTo */
            qname = axutil_qname_create(env, ELEM_NAME_ENDTO, EVENTING_NAMESPACE, NULL);
            endto_elem = axiom_element_get_first_child_with_qname(sub_elem, env, qname,
                sub_node, &endto_node);
            axutil_qname_free(qname, env);

            endto = axiom_element_get_text(endto_elem, env, endto_node);

            endto_epr = axis2_endpoint_ref_create(env, endto);

            savan_subscriber_set_end_to(subscriber, env, endto_epr);

            /* Get Delivery element and read NotifyTo */
            qname = axutil_qname_create(env, ELEM_NAME_DELIVERY, EVENTING_NAMESPACE, NULL);
            delivery_elem = axiom_element_get_first_child_with_qname(sub_elem, env, qname,
                sub_node, &delivery_node);
            axutil_qname_free(qname, env);

            qname = axutil_qname_create(env, ELEM_NAME_NOTIFYTO, EVENTING_NAMESPACE, NULL);
            notify_elem = axiom_element_get_first_child_with_qname(delivery_elem, env, qname,
                delivery_node, &notify_node);
            axutil_qname_free(qname, env);
            notify = axiom_element_get_text(notify_elem, env, notify_node);

            notify_epr = axis2_endpoint_ref_create(env, notify);

            savan_subscriber_set_notify_to(subscriber, env, notify_epr);

            /* Expires */
            qname = axutil_qname_create(env, ELEM_NAME_EXPIRES, EVENTING_NAMESPACE, NULL);
            expires_elem = axiom_element_get_first_child_with_qname(sub_elem, env, qname,
                sub_node, &expires_node);
            axutil_qname_free(qname, env);

            expires = axiom_element_get_text(expires_elem, env, expires_node);

            savan_subscriber_set_expires(subscriber, env, expires);

            /* Filter */
            qname = axutil_qname_create(env, ELEM_NAME_FILTER, EVENTING_NAMESPACE, NULL);
            filter_elem = axiom_element_get_first_child_with_qname(sub_elem, env, qname,
                sub_node, &filter_node);
            axutil_qname_free(qname, env);

            filter = axiom_element_get_text(filter_elem, env, filter_node);

            savan_subscriber_set_filter(subscriber, env, filter);
            axutil_hash_set(subscriber_list, savan_subscriber_get_id(subscriber, 
                env), AXIS2_HASH_KEY_STRING, subscriber);
        }
    }
    AXIS2_LOG_DEBUG(env->log, AXIS2_LOG_SI, 
        "[SAVAN] End:process_subscriber_list_node");
    return subscriber_list;
}

static axiom_node_t *
build_add_subscriber_om_payload(
    const axutil_env_t *env,
    savan_subscriber_t *subscriber)
{
    axiom_node_t *add_node = NULL;
    axiom_element_t* add_ele = NULL;
    axiom_namespace_t *ns = NULL;
    axiom_namespace_t *ns1 = NULL;
    axiom_node_t *sub_node = NULL;
    axiom_node_t *id_node = NULL;
    axiom_node_t *topic_node = NULL;
    axiom_node_t *endto_node = NULL;
    axiom_node_t *delivery_node = NULL;
    axiom_node_t *notify_node = NULL;
    axiom_node_t *filter_node = NULL;
    axiom_node_t *expires_node = NULL;
    axiom_element_t* sub_elem = NULL;
    axiom_element_t* id_elem = NULL;
    axiom_element_t* topic_elem = NULL;
    axiom_element_t* endto_elem = NULL;
    axiom_element_t* delivery_elem = NULL;
    axiom_element_t* notify_elem = NULL;
    axiom_element_t* filter_elem = NULL;
    axiom_element_t* expires_elem = NULL;
    const axis2_char_t *endto = NULL;
    const axis2_char_t *notify = NULL;
    axis2_char_t *filter = NULL;
    const axis2_char_t *expires = NULL;
    axis2_char_t *topic = NULL;
    axis2_char_t *id = NULL;
	axis2_endpoint_ref_t *notify_ref = NULL;
    axis2_endpoint_ref_t *endto_ref = savan_subscriber_get_end_to(subscriber, env);
    endto = axis2_endpoint_ref_get_address(endto_ref, env);
    notify_ref = savan_subscriber_get_notify_to(subscriber, env);
    notify = axis2_endpoint_ref_get_address(notify_ref, env);
    filter = savan_subscriber_get_filter(subscriber, env); 
    expires = savan_subscriber_get_expires(subscriber, env); 
    id = savan_subscriber_get_id(subscriber, env);

    ns = axiom_namespace_create (env, EVENTING_NAMESPACE, EVENTING_NS_PREFIX);
    ns1 = axiom_namespace_create (env, SAVAN_NAMESPACE, SAVAN_NS_PREFIX);
    add_ele = axiom_element_create(env, NULL, ELEM_NAME_ADD_SUBSCRIBER, ns1, &add_node);
    
    /* create the id element */
    if(id)
    {
        id_elem = axiom_element_create(env, add_node, ELEM_NAME_ID, ns1, &id_node);
            axiom_element_set_text(id_elem, env, id, id_node);
    }
    /* create the topic element */
    topic_elem = axiom_element_create(env, add_node, ELEM_NAME_TOPIC, ns1, &topic_node);
    topic = savan_subscriber_get_topic(subscriber, env);
    if(topic)
        axiom_element_set_text(topic_elem, env, topic, topic_node);
    /* create the subscriber element */
    sub_elem = axiom_element_create(env, add_node, ELEM_NAME_SUBSCRIBE, ns, &sub_node);
    
    /* EndTo element */
    endto_elem = axiom_element_create(env, sub_node, ELEM_NAME_ENDTO, ns,
        &endto_node);
    axiom_element_set_text(endto_elem, env, endto, endto_node);
    
    /* Delivery element */
    delivery_elem = axiom_element_create(env, sub_node, ELEM_NAME_DELIVERY, ns,
        &delivery_node);
        
    notify_elem = axiom_element_create(env, delivery_node, ELEM_NAME_NOTIFYTO, ns,
        &notify_node);
    axiom_element_set_text(notify_elem, env, notify, notify_node);
    
    /* Expires element */
    expires_elem = axiom_element_create(env, sub_node, ELEM_NAME_EXPIRES, ns,
        &expires_node);
    axiom_element_set_text(expires_elem, env, expires, expires_node);
    /* Filter element */
    filter_elem = axiom_element_create(env, sub_node, ELEM_NAME_FILTER, ns,
        &endto_node);
    axiom_element_set_text(filter_elem, env, filter, filter_node);
    
    return add_node;
}

static axiom_node_t *
build_remove_subscriber_om_payload(
    const axutil_env_t *env,
    savan_subscriber_t *subscriber)
{
    axiom_node_t *remove_node = NULL;
    axiom_element_t* remove_ele = NULL;
    axiom_namespace_t *ns = NULL;
    axiom_namespace_t *ns1 = NULL;
    axiom_node_t *id_node = NULL;
    axiom_node_t *topic_node = NULL;
    axiom_element_t* id_elem = NULL;
    axiom_element_t* topic_elem = NULL;
    axis2_char_t *topic = NULL;
    axis2_char_t *id = NULL;

    id = savan_subscriber_get_id(subscriber, env);

    ns = axiom_namespace_create (env, EVENTING_NAMESPACE, EVENTING_NS_PREFIX);
    ns1 = axiom_namespace_create (env, SAVAN_NAMESPACE, SAVAN_NS_PREFIX);
    remove_ele = axiom_element_create(env, NULL, ELEM_NAME_REMOVE_SUBSCRIBER, 
        ns1, &remove_node);
    
    /* create the id element */
    if(id)
    {
        id_elem = axiom_element_create(env, remove_node, ELEM_NAME_ID, ns1, &id_node);
            axiom_element_set_text(id_elem, env, id, id_node);
    }
    /* create the topic element */
    topic_elem = axiom_element_create(env, remove_node, ELEM_NAME_TOPIC, ns1, &topic_node);
    topic = savan_subscriber_get_topic(subscriber, env);
    if(topic)
        axiom_element_set_text(topic_elem, env, topic, topic_node);
    
    return remove_node;
}

/******************************************************************************/

axis2_char_t * AXIS2_CALL
savan_util_get_expiry_time(
    const axutil_env_t *env)
{
    /* TODO: decide how to set expiry time */
    
    return NULL;
}

/******************************************************************************/

axis2_char_t * AXIS2_CALL
savan_util_get_renewed_expiry_time(
    const axutil_env_t *env,
    axis2_char_t *expiry)
{
    /* TODO: decide how to renew expiry time */
    
    return NULL;
}

axis2_char_t *AXIS2_CALL
savan_util_get_topic_name_from_topic_url(
    const axutil_env_t *env,
    axis2_char_t *topic_url)
{
    axis2_char_t *topic = NULL;
    topic  = axutil_strdup(env, axutil_rindex(topic_url, '/') + 1);
    return topic;
}

axutil_array_list_t *AXIS2_CALL
savan_util_get_topic_list_from_remote_subs_mgr(
    const axutil_env_t *env,
    axis2_char_t *subs_mgr_url,
    void *s_client)
{
    axis2_endpoint_ref_t* endpoint_ref = NULL;
    axis2_options_t *options = NULL;
    axis2_svc_client_t* svc_client = NULL;
    axiom_node_t *payload = NULL;
    axiom_node_t *ret_node = NULL;
    axutil_array_list_t *topic_list = NULL;

    AXIS2_LOG_DEBUG(env->log, AXIS2_LOG_SI, 
        "[SAVAN] Start:savan_util_get_topic_list_from_remote_subs_mgr");
    if(!s_client)
    {
        svc_client = (axis2_svc_client_t *) savan_util_get_svc_client(env);
    }
    else
    {
        svc_client = (axis2_svc_client_t *) s_client;
    }
    options = (axis2_options_t *) axis2_svc_client_get_options(svc_client, env);
    endpoint_ref = axis2_endpoint_ref_create(env, subs_mgr_url);
    axis2_options_set_to(options, env, endpoint_ref);
    
    payload = build_topics_request_om_payload(env);
    ret_node = axis2_svc_client_send_receive(svc_client, env, payload);
    if (ret_node)
    {
        topic_list = process_topic_list_node(env, ret_node);
    }
    else
    {
        AXIS2_LOG_ERROR(env->log, AXIS2_LOG_SI, 
            "[SAVAN] Stub invoke FAILED: Error code:"
            " %d :: %s", env->error->error_number,
            AXIS2_ERROR_GET_MESSAGE(env->error));
    }
    if(!s_client && svc_client)
    {
        /*axis2_svc_client_free(svc_client, env);*/
    }
    AXIS2_LOG_DEBUG(env->log, AXIS2_LOG_SI, 
        "[SAVAN] End:savan_util_get_topic_list_from_remote_subs_mgr");
    return topic_list;
}

static axiom_node_t *
build_topics_request_om_payload(
    const axutil_env_t *env)
{
    axiom_node_t *om_node = NULL;
    axiom_element_t* om_ele = NULL;
    axiom_namespace_t *ns1 = NULL;
    axis2_char_t *om_str = NULL;

    ns1 = axiom_namespace_create (env, SAVAN_NAMESPACE, SAVAN_NS_PREFIX);
    om_ele = axiom_element_create(env, NULL, ELEM_NAME_GET_TOPIC_LIST, ns1, &om_node);
    om_str = axiom_node_to_string(om_node, env);
    if (om_str)
    {
        AXIS2_LOG_DEBUG(env->log, AXIS2_LOG_SI, 
            "Sending topics_request_om_payload: %s", om_str);
        AXIS2_FREE(env->allocator, om_str);
        om_str =  NULL;
    }
    return om_node;
}

static axutil_array_list_t *
process_topic_list_node(
    const axutil_env_t *env,
    axiom_node_t *topic_list_node)
{
    axiom_element_t *topic_list_element = NULL;
    axiom_children_qname_iterator_t *topic_iter = NULL;
    axutil_qname_t *qname = NULL;
    axutil_array_list_t *topic_list = NULL;

    AXIS2_LOG_DEBUG(env->log, AXIS2_LOG_SI, 
        "[SAVAN] Start:process_topic_list_node");
    topic_list_element = axiom_node_get_data_element(topic_list_node, env); 
         
    /* Get topic elements from topic list */
    qname = axutil_qname_create(env, ELEM_NAME_TOPIC, SAVAN_NAMESPACE, 
        NULL);
    topic_iter = axiom_element_get_children_with_qname(topic_list_element, env,
        qname, topic_list_node);
    axutil_qname_free(qname, env);
    if(!topic_iter)
    {
        AXIS2_LOG_DEBUG(env->log, AXIS2_LOG_SI, 
            "[SAVAN] Topic list is empty");
        return NULL;
    }
    if(axiom_children_qname_iterator_has_next(topic_iter, env))
    {
        topic_list = axutil_array_list_create(env, 0);
    }

    while(axiom_children_qname_iterator_has_next(topic_iter, env))
    {
        axiom_node_t *topic_node = NULL;
        axiom_element_t *topic_elem = NULL;
        axis2_char_t *topic_url_str = NULL;

        topic_node = axiom_children_qname_iterator_next(topic_iter, env);
        if(topic_node)
        {
            topic_elem = axiom_node_get_data_element(topic_node, env);
            topic_url_str = axiom_element_get_text(topic_elem, env, topic_node);
            axutil_array_list_add(topic_list, env, axutil_strdup(env, 
                topic_url_str));
            AXIS2_LOG_DEBUG(env->log, AXIS2_LOG_SI, "topic_url_str:%s", 
                topic_url_str);
        }
    }
    AXIS2_LOG_DEBUG(env->log, AXIS2_LOG_SI, 
        "[SAVAN] End:process_topic_list_node");
    return topic_list;
}

void *AXIS2_CALL
savan_util_get_svc_client(
    const axutil_env_t *env)
{
    const axis2_char_t *client_home = NULL;
    axis2_options_t *options = NULL;
    axis2_svc_client_t *svc_client = NULL;

    client_home = AXIS2_GETENV("AXIS2C_HOME");
    if (!client_home)
        client_home = "../../deploy";
    options = axis2_options_create(env);
    axis2_options_set_xml_parser_reset(options, env, AXIS2_FALSE);
    svc_client = axis2_svc_client_create(env, client_home);
    if (!svc_client)
    {
        AXIS2_LOG_ERROR(env->log, AXIS2_LOG_SI, 
            "[SAVAN] Stub invoke FAILED: Error code:"
            " %d :: %s", env->error->error_number,
            AXIS2_ERROR_GET_MESSAGE(env->error));
        return NULL;
    }
    axis2_svc_client_set_options(svc_client, env, options);    
    axis2_svc_client_engage_module(svc_client, env, AXIS2_MODULE_ADDRESSING);
    return svc_client;
}

