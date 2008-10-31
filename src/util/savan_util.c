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
#include <axis2_engine.h>
#include <axis2_core_utils.h>
#include <axis2_endpoint_ref.h>
#include <platforms/axutil_platform_auto_sense.h>
#include <axiom_soap.h>
#include <axiom_soap_const.h>
#include <axiom_soap_envelope.h>
#include <axiom_element.h>
#include <axiom_node.h>

#include <savan_util.h>
#include <savan_msg_recv.h>
#include <savan_error.h>
#include <savan_db_mgr.h>
#ifdef SAVAN_FILTERING
#include <libxslt/xsltutils.h>
#endif

#ifdef SAVAN_FILTERING
axis2_status_t
savan_util_update_filter_template(
    xmlNodeSetPtr nodes,
    const xmlChar* value);
#endif

static axis2_status_t
add_subscriber_to_remote_subs_mgr(
    const axutil_env_t *env,
    savan_subscriber_t *subscriber,
    axis2_char_t *subs_mgr_url);

static axiom_node_t *
build_add_subscriber_om_payload(
    const axutil_env_t *env,
    savan_subscriber_t *subscriber);

static axiom_node_t *
build_subscriber_request_om_payload(
    const axutil_env_t *env,
    axis2_char_t *subs_id);

static axiom_node_t *
build_subscribers_request_om_payload(
    const axutil_env_t *env,
    axis2_char_t *topic);

static axiom_node_t *
build_topics_request_om_payload(
    const axutil_env_t *env);

static savan_subscriber_t *
savan_util_process_savan_specific_subscriber_node(
    const axutil_env_t *env,
    axiom_node_t *sub_node);

static axutil_array_list_t *
process_subscriber_list_node(
    const axutil_env_t *env,
    axiom_node_t *subs_list_node);

static axutil_array_list_t *
process_topic_list_node(
    const axutil_env_t *env,
    axiom_node_t *subs_list_node);

axis2_status_t AXIS2_CALL
savan_util_create_fault_envelope(
    axis2_msg_ctx_t *msg_ctx,
    const axutil_env_t *env,
    axis2_char_t *code,
    axis2_char_t *subcode,
    axis2_char_t *reason,
    axis2_char_t *detail)
{

    axiom_soap_envelope_t *envelope = NULL;
    /*axiom_node_t* detail_om_node = NULL;
    axiom_element_t * detail_om_ele = NULL;
    axis2_msg_info_headers_t* info_header = NULL;
    int soap_version = AXIOM_SOAP12;
    axutil_array_list_t *sub_codes = NULL;
    axiom_namespace_t *soap_ns = NULL;
    axiom_namespace_t *ns1 = NULL;*/
    axiom_soap_body_t *body = NULL;
    axiom_node_t *body_node = NULL;
    axiom_node_t *fault_node = NULL;


    fault_node = savan_util_build_fault_msg(env, code, subcode, reason, detail);
    envelope = axiom_soap_envelope_create_default_soap_envelope(env,
        AXIOM_SOAP12);

    /*info_header =  axis2_msg_ctx_get_msg_info_headers(msg_ctx, env);
    axis2_msg_info_headers_set_action(info_header, env, SAVAN_ACTIONS_FAULT);

    axis2_msg_ctx_set_msg_info_headers(msg_ctx, env, info_header);*/

    body = axiom_soap_envelope_get_body(envelope, env);
    body_node = axiom_soap_body_get_base_node(body, env);

    fault_node = savan_util_build_fault_msg(env, code,
        subcode, reason, detail);

    axiom_node_add_child(body_node , env, fault_node);
    axis2_msg_ctx_set_fault_soap_envelope(msg_ctx, env, envelope);

    return AXIS2_SUCCESS;
}

#ifdef SAVAN_FILTERING
axis2_status_t AXIS2_CALL
savan_util_set_filter_template_for_subscriber(
    savan_subscriber_t *subscriber,
    const axutil_env_t *env)
{
    xsltStylesheetPtr xslt_template_xslt = NULL;
    xmlDocPtr xslt_template_xml = NULL;
    axis2_char_t *filter_template_path = NULL;

	if(!savan_subscriber_get_filter(subscriber, env))
	{
		return AXIS2_SUCCESS;
	}

    filter_template_path = savan_subscriber_get_filter_template_path(subscriber, env);
    if(!filter_template_path)
    {
        AXIS2_LOG_ERROR(env->log, AXIS2_LOG_SI, "[savan] Filter template path not set");
        return AXIS2_FAILURE;
    }

    AXIS2_LOG_DEBUG(env->log, AXIS2_LOG_SI, "[savan] filter_template_path:%s", filter_template_path);

    xslt_template_xml = xmlParseFile(filter_template_path);
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
}
#endif

#ifdef SAVAN_FILTERING
axis2_status_t AXIS2_CALL
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
    axiom_xml_reader_t *reader = NULL;
    axiom_stax_builder_t *om_builder = NULL;
    axiom_document_t *document = NULL;
    axiom_node_t *node = NULL;

	if(!savan_subscriber_get_filter(subscriber, env))
	{
		return AXIS2_SUCCESS;
	}

    payload_string = axiom_node_to_string(payload, env);
    AXIS2_LOG_DEBUG(env->log, AXIS2_LOG_SI, 
        "[savan] payload_string before applying filter:%s", payload_string);

    payload_doc = (xmlDocPtr)xmlParseDoc((xmlChar*)payload_string);

    #ifdef SAVAN_FILTERING
        savan_util_set_filter_template_for_subscriber(subscriber, env);
	#endif

    xslt_template_filter = (xsltStylesheetPtr)savan_subscriber_get_filter_template(subscriber,
        env);

    xmlDocPtr result_doc = (xmlDocPtr)xsltApplyStylesheet(xslt_template_filter, payload_doc, NULL);

    if(result_doc)
    {
        xmlDocDumpMemory(result_doc, &buffer, &size);
    }

    if(buffer)
    {
        reader = axiom_xml_reader_create_for_memory(env, 
                (char*)buffer,axutil_strlen((char*)buffer), NULL, AXIS2_XML_PARSER_TYPE_BUFFER);
    }

    if(reader)
    {
        om_builder = axiom_stax_builder_create(env, reader);
    }

    if(om_builder)
    {
        document = axiom_stax_builder_get_document(om_builder, env);
    }

    if(document)
    {
        node = axiom_document_build_all(document, env);
    }

    if(om_builder)
    {
        axiom_stax_builder_free_self(om_builder, env);
    }

    /*free(payload_string);*/ /* In apache freeing this give seg fault:damitha */
    if(result_doc)
    {
	    xmlFreeDoc(result_doc);
    }

	if(!node)
	{
		return AXIS2_FAILURE;
	}
	else
	{
		return AXIS2_SUCCESS;
	}
}
#endif

#ifdef SAVAN_FILTERING
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
}
#endif

axiom_node_t * AXIS2_CALL
savan_util_build_fault_msg(
    const axutil_env_t *env,
    axis2_char_t * code,
    axis2_char_t * subcode,
    axis2_char_t * reason,
    axis2_char_t * detail)
{
    axiom_node_t *fault_node = NULL;
    axiom_element_t *fault_ele = NULL;
    axiom_node_t *code_node = NULL;
    axiom_element_t *code_ele = NULL;
    axiom_node_t *code_value_node = NULL;
    axiom_element_t *code_value_ele = NULL;
    axiom_node_t *sub_code_node = NULL;
    axiom_element_t *sub_code_ele = NULL;
    axiom_node_t *sub_code_value_node = NULL;
    axiom_element_t *sub_code_value_ele = NULL;
    axiom_node_t *reason_node = NULL;
    axiom_element_t *reason_ele = NULL;
    axiom_node_t *reason_text_node = NULL;
    axiom_element_t *reason_text_ele = NULL;
    axiom_node_t *detail_node = NULL;
    axiom_element_t *detail_ele = NULL;

    fault_ele = axiom_element_create(env, NULL, "Fault", NULL, &fault_node);

   	code_ele = axiom_element_create(env, fault_node, "Code",
        NULL, &code_node);
	code_value_ele = axiom_element_create(env, 
		code_node, "Value", NULL, &code_value_node);
   	axiom_element_set_text(code_value_ele, env, code, code_value_node);
	sub_code_ele = axiom_element_create(env, 
		code_node, "Subcode", NULL, &sub_code_node);
	sub_code_value_ele = axiom_element_create(env, 
		sub_code_node, "Value", NULL, &sub_code_value_node);
   	axiom_element_set_text(sub_code_value_ele, env, subcode, sub_code_value_node);
	reason_ele = axiom_element_create(env, fault_node, "Reason", NULL, &reason_node);
	reason_text_ele = axiom_element_create(env, 
		reason_node, "Text", NULL, &reason_text_node);
	axiom_element_set_text(reason_text_ele, env, reason, reason_text_node);
	detail_ele = axiom_element_create(env, fault_node, "Detail", NULL, &detail_node);	
	axiom_element_set_text(detail_ele, env, detail, detail_node);
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
        AXIS2_LOG_ERROR(env->log, AXIS2_LOG_SI, "[savan] Cannot extract message info headers"); 
        return SAVAN_MSG_TYPE_UNKNOWN;
    }
    
    action = axis2_msg_info_headers_get_action(info_header, env);
    if( ! action)
    {
        AXIS2_LOG_ERROR(env->log, AXIS2_LOG_SI, "[savan] Cannot extract soap action"); 
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
    
    AXIS2_LOG_TRACE(env->log, AXIS2_LOG_SI, 
        "[savan] Entry:savan_util_get_subscription_id_from_msg");
    
    /* Get soap envelop and extract the subscription id */

    envelope =  axis2_msg_ctx_get_soap_envelope(msg_ctx, env);
    if (!envelope)
    {
        AXIS2_LOG_ERROR(env->log, AXIS2_LOG_SI, "[savan] Failed to extract the soap envelop");
        return NULL;
    }
    
    header = axiom_soap_envelope_get_header(envelope, env);
    if (!header)
    {
        AXIS2_LOG_ERROR(env->log, AXIS2_LOG_SI, 
            "[savan] Failed to extract the soap header"); 
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
    
    AXIS2_LOG_TRACE(env->log, AXIS2_LOG_SI, 
        "[savan] Exit:savan_util_get_subscription_id_from_msg");
    return sub_id;    
}

savan_subscriber_t * AXIS2_CALL
savan_util_get_subscriber_from_msg(
        const axutil_env_t *env,
        axis2_msg_ctx_t *msg_ctx,
        axis2_char_t *sub_id)
{
    savan_subscriber_t *subscriber = NULL;

    AXIS2_LOG_TRACE(env->log, AXIS2_LOG_SI, "[savan] Entry:savan_util_get_subscriber_from_msg");

    /* Extract subscription id from msg if not already given */
    if (!sub_id)
    {
        sub_id = savan_util_get_subscription_id_from_msg(env, msg_ctx);
    }
    { 
        axis2_char_t sql_retrieve[256];
        axis2_conf_ctx_t *conf_ctx = axis2_msg_ctx_get_conf_ctx(msg_ctx, env);
        axis2_conf_t *conf = axis2_conf_ctx_get_conf(conf_ctx, env);

        sprintf(sql_retrieve, "select id, end_to, notify_to, delivery_mode, "\
            "expires, filter, renewed, topic_url from subscriber, topic"\
            " where id='%s' and topic.topic_name=subscriber.topic_name;", sub_id);

        subscriber = savan_db_mgr_retrieve(env, savan_util_get_dbname(env, conf), 
        savan_db_mgr_subs_retrieve_callback, sql_retrieve);
    }

    AXIS2_LOG_TRACE(env->log, AXIS2_LOG_SI, "[savan] Entry:savan_util_get_subscriber_from_msg");
    
    return subscriber;
}

axis2_status_t AXIS2_CALL
savan_util_add_subscriber(
    const axutil_env_t *env,
    axis2_msg_ctx_t *msg_ctx,
    savan_subscriber_t *subscriber)
{
    axis2_conf_ctx_t *conf_ctx = NULL;
    axis2_conf_t *conf = NULL;
    axis2_module_desc_t *module_desc = NULL;
    axis2_svc_t *pubs_svc = NULL;
    axutil_param_t *param = NULL;
    axis2_endpoint_ref_t *topic_epr = NULL;
    axis2_char_t *topic_url = NULL;
    axis2_char_t *topic = NULL;
    axis2_status_t status = AXIS2_FAILURE;
    axutil_qname_t *qname = NULL;

    AXIS2_LOG_TRACE(env->log, AXIS2_LOG_SI, "[savan] Entry:savan_util_add_subscriber");
    conf_ctx = axis2_msg_ctx_get_conf_ctx(msg_ctx, env);
    topic_epr = axis2_msg_ctx_get_to(msg_ctx, env);
    topic_url = (axis2_char_t *) axis2_endpoint_ref_get_address(topic_epr, 
        env);
    topic = savan_util_get_topic_name_from_topic_url(env, topic_url);
    pubs_svc = axis2_msg_ctx_get_svc(msg_ctx, env);
    if (!pubs_svc)
    {
        AXIS2_LOG_ERROR(env->log, AXIS2_LOG_SI, 
            "[savan] Failed to extract the %s publisher service", topic); 
        return AXIS2_FAILURE;
    }
    conf = axis2_conf_ctx_get_conf(conf_ctx, env);
    qname = axutil_qname_create(env, SAVAN_MODULE, NULL, NULL);
    module_desc = axis2_conf_get_module(conf, env, qname);
    param = axis2_module_desc_get_param(module_desc, env, SAVAN_SUBSCRIPTION_MGR_URL);
    axutil_qname_free(qname, env);
    if(param)
    {
        axis2_char_t *subs_mgr_url = NULL;
        subs_mgr_url = axutil_param_get_value(param, env);
        status = add_subscriber_to_remote_subs_mgr(env, subscriber, subs_mgr_url);
    }
    else
    {
        axis2_conf_ctx_t *conf_ctx = axis2_msg_ctx_get_conf_ctx(msg_ctx, env);
        axis2_conf_t *conf = axis2_conf_ctx_get_conf(conf_ctx, env);

        status = savan_db_mgr_insert_subscriber(env, savan_util_get_dbname(env, conf), subscriber);
    }
    if(status)
    {
        axutil_property_t *subs_prop = NULL;
        subs_prop = axutil_property_create_with_args(env, 0, 0, 
            savan_subscriber_free_void_arg, subscriber);
        axis2_msg_ctx_set_property(msg_ctx, env, SAVAN_SUBSCRIBER, subs_prop);
    }

    AXIS2_LOG_TRACE(env->log, AXIS2_LOG_SI, "[savan] Exit:savan_util_add_subscriber"); 
    return status;
}

axis2_status_t AXIS2_CALL
savan_util_update_subscriber(
    const axutil_env_t *env,
    axis2_msg_ctx_t *msg_ctx,
    savan_subscriber_t *subscriber)
{
    axis2_conf_ctx_t *conf_ctx = NULL;

    AXIS2_LOG_TRACE(env->log, AXIS2_LOG_SI, "[savan] Entry:savan_util_update_subscriber");

    conf_ctx = axis2_msg_ctx_get_conf_ctx(msg_ctx, env);
    {
        axis2_conf_ctx_t *conf_ctx = axis2_msg_ctx_get_conf_ctx(msg_ctx, env);
        axis2_conf_t *conf = axis2_conf_ctx_get_conf(conf_ctx, env);

        savan_db_mgr_update_subscriber(env, savan_util_get_dbname(env, conf), subscriber);
    }

    AXIS2_LOG_TRACE(env->log, AXIS2_LOG_SI, "[savan] Exit:savan_util_update_subscriber"); 
    return AXIS2_SUCCESS;
}

axis2_status_t AXIS2_CALL
savan_util_remove_subscriber(
    const axutil_env_t *env,
    axis2_msg_ctx_t *msg_ctx,
    savan_subscriber_t *subscriber)
{
    AXIS2_LOG_TRACE(env->log, AXIS2_LOG_SI, "[savan] Entry:savan_util_remove_subscriber");

    {
        axis2_char_t *subs_id = NULL;
        axis2_char_t sql_remove[256];
        axis2_conf_ctx_t *conf_ctx = axis2_msg_ctx_get_conf_ctx(msg_ctx, env);
        axis2_conf_t *conf = axis2_conf_ctx_get_conf(conf_ctx, env);

        subs_id = savan_subscriber_get_id(subscriber, env);

        /* Extract the store from the svc and remove the given subscriber */

        sprintf(sql_remove, "delete from subscriber where id='%s'", subs_id);

        savan_db_mgr_remove(env, savan_util_get_dbname(env, conf), sql_remove);
    }

    AXIS2_LOG_TRACE(env->log, AXIS2_LOG_SI, "[savan] Exit:savan_util_remove_subscriber");
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
    
    AXIS2_LOG_TRACE(env->log, AXIS2_LOG_SI, "[savan] Entry:set_sub_store");
    
    /* Create a hash map */
    store = axutil_hash_make(env);
    if (!store)
    {
        /* TODO : error reporting */
        AXIS2_LOG_ERROR(env->log, AXIS2_LOG_SI, "[savan] Could not create subscriber store");
        return AXIS2_FAILURE;
    }
    
    /* Add the hash map as a parameter to the given service */
    param = axutil_param_create(env, store_name, (void*)store);
    if (!param)
    {
        /* TODO : error reporting */
        AXIS2_LOG_ERROR(env->log, AXIS2_LOG_SI, "[savan] Could not create subscriber store param");
        return AXIS2_FAILURE;
    }
    
    axis2_svc_add_param(svc, env, param);
    AXIS2_LOG_TRACE(env->log, AXIS2_LOG_SI, "[savan] Exit:set_sub_store");
    
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

/*static axis2_status_t
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
    // Send request
    axis2_svc_client_send_robust(svc_client, env, payload);
    if(svc_client)
        axis2_svc_client_free(svc_client, env);

    return AXIS2_SUCCESS;
}*/

savan_subscriber_t *AXIS2_CALL
savan_util_get_subscriber_from_remote_subs_mgr(
    const axutil_env_t *env,
    axis2_char_t *subs_id,
    axis2_char_t *subs_mgr_url,
    void *s_client)
{
    axis2_endpoint_ref_t* endpoint_ref = NULL;
    axis2_options_t *options = NULL;
    axis2_svc_client_t* svc_client = NULL;
    axiom_node_t *payload = NULL;
    axiom_node_t *ret_node = NULL;
    savan_subscriber_t *subscriber = NULL;

    AXIS2_LOG_TRACE(env->log, AXIS2_LOG_SI, 
        "[savan] Entry:savan_util_get_subscriber_from_remote_subs_mgr");

    svc_client = (axis2_svc_client_t *) s_client;
    options = (axis2_options_t *)axis2_svc_client_get_options(svc_client, env);
    endpoint_ref = axis2_endpoint_ref_create(env, subs_mgr_url);
    axis2_options_set_to(options, env, endpoint_ref);
    
    payload = build_subscriber_request_om_payload(env, subs_id);
    ret_node = axis2_svc_client_send_receive(svc_client, env, payload);
    if (ret_node)
    {
        subscriber = savan_util_process_savan_specific_subscriber_node(env, ret_node);
    }
    else
    {
        AXIS2_LOG_ERROR(env->log, AXIS2_LOG_SI, "[savan] Stub invoke FAILED: Error code:"
            " %d :: %s", env->error->error_number, AXIS2_ERROR_GET_MESSAGE(env->error));
    }

    AXIS2_LOG_TRACE(env->log, AXIS2_LOG_SI, 
            "[savan] Exit:savan_util_get_subscriber_from_remote_subs_mgr");
    return subscriber;
}

static axiom_node_t *
build_subscriber_request_om_payload(
    const axutil_env_t *env,
    axis2_char_t *subs_id)
{
    axiom_node_t *om_node = NULL;
    axiom_element_t* om_ele = NULL;
    axiom_node_t* subs_id_om_node = NULL;
    axiom_element_t * subs_id_om_ele = NULL;
    axiom_namespace_t *ns1 = NULL;
    axis2_char_t *om_str = NULL;

    ns1 = axiom_namespace_create (env, SAVAN_NAMESPACE, SAVAN_NS_PREFIX);
    om_ele = axiom_element_create(env, NULL, ELEM_NAME_GET_SUBSCRIBER, ns1, &om_node);
    subs_id_om_ele = axiom_element_create(env, om_node, ELEM_NAME_SUBSCRIBER_ID, ns1, 
        &subs_id_om_node);
    axiom_element_set_text(subs_id_om_ele, env, subs_id, subs_id_om_node);

    om_str = axiom_node_to_string(om_node, env);
    if (om_str)
    {
        AXIS2_LOG_DEBUG(env->log, AXIS2_LOG_SI, "Sending OM : %s", om_str);
        AXIS2_FREE(env->allocator, om_str);
        om_str =  NULL;
    }
    return om_node;
}

static savan_subscriber_t *
savan_util_process_savan_specific_subscriber_node(
    const axutil_env_t *env,
    axiom_node_t *subs_node)
{
    axiom_element_t *subs_elem = NULL;
    axiom_node_t *sub_node = NULL;
    axiom_element_t *sub_elem = NULL;
    axutil_qname_t *qname = NULL;
    axiom_node_t *id_node = NULL;
    axiom_element_t *id_elem = NULL;
    axis2_char_t *id = NULL;
    savan_subscriber_t *subscriber = NULL;
    axis2_status_t status = AXIS2_SUCCESS;

    AXIS2_LOG_TRACE(env->log, AXIS2_LOG_SI, 
            "[savan] Entry:savan_util_process_savan_specific_subscriber_node");

    AXIS2_PARAM_CHECK(env->error, subs_node, AXIS2_FAILURE);

    subscriber = savan_subscriber_create(env);
    if (!subscriber)
    {
        AXIS2_LOG_ERROR(env->log, AXIS2_LOG_SI, "[savan] Failed to create a subscriber instance");
        return NULL;
    }

    subs_elem = axiom_node_get_data_element(subs_node, env); 

    /* Id */
    qname = axutil_qname_create(env, ELEM_NAME_ID, SAVAN_NAMESPACE, NULL);
    id_elem = axiom_element_get_first_child_with_qname(subs_elem, env, qname, subs_node, &id_node);
    axutil_qname_free(qname, env);
    id = axiom_element_get_text(id_elem, env, id_node);
    savan_subscriber_set_id(subscriber, env, id);
    AXIS2_LOG_DEBUG(env->log, AXIS2_LOG_SI, "[savan] Received subscriber id:%s", id);
    
    qname = axutil_qname_create(env, ELEM_NAME_SUBSCRIBE, EVENTING_NAMESPACE, NULL);
    sub_elem = axiom_element_get_first_child_with_qname(subs_elem, env, qname, subs_node, &sub_node);
    axutil_qname_free(qname, env);
    
    if(sub_node)
    {
        /* Now read each sub element of Subscribe element */
        status = savan_util_process_subscriber_node(env, sub_node, sub_elem, subscriber);
        if(AXIS2_SUCCESS != status)
        {
            AXIS2_LOG_ERROR(env->log, AXIS2_LOG_SI, "[savan] Parsing subscriber node failed");
            axutil_error_set_status_code(env->error, AXIS2_FAILURE);
            return NULL;
        }
    }

    AXIS2_LOG_TRACE(env->log, AXIS2_LOG_SI, 
            "[savan] Exit:savan_util_process_savan_specific_subscriber_node");
    return subscriber;
}

axutil_array_list_t *AXIS2_CALL
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
    axutil_array_list_t *subscriber_list = NULL;

    AXIS2_LOG_TRACE(env->log, AXIS2_LOG_SI, 
        "[savan] Entry:savan_util_get_subscriber_list_from_remote_subs_mgr");

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
            "[savan] Stub invoke FAILED: Error code:"
            " %d :: %s", env->error->error_number,
            AXIS2_ERROR_GET_MESSAGE(env->error));
    }
    AXIS2_LOG_TRACE(env->log, AXIS2_LOG_SI, 
        "[savan] Exit:savan_util_get_subscriber_list_from_remote_subs_mgr");
    return subscriber_list;
}

static axiom_node_t *
build_subscribers_request_om_payload(
    const axutil_env_t *env,
    axis2_char_t *topic)
{
    axiom_node_t *om_node = NULL;
    axiom_element_t* om_ele = NULL;
    axiom_node_t* topic_om_node = NULL;
    axiom_element_t * topic_om_ele = NULL;
    axiom_namespace_t *ns1 = NULL;
    axis2_char_t *om_str = NULL;

    ns1 = axiom_namespace_create (env, SAVAN_NAMESPACE, SAVAN_NS_PREFIX);
    om_ele = axiom_element_create(env, NULL, ELEM_NAME_GET_SUBSCRIBER_LIST, ns1, &om_node);
    topic_om_ele = axiom_element_create(env, om_node, ELEM_NAME_TOPIC, ns1, 
        &topic_om_node);
    axiom_element_set_text(topic_om_ele, env, topic, topic_om_node);

    om_str = axiom_node_to_string(om_node, env);
    if (om_str)
    {
        AXIS2_LOG_DEBUG(env->log, AXIS2_LOG_SI, "Sending OM : %s", om_str);
        AXIS2_FREE(env->allocator, om_str);
        om_str =  NULL;
    }
    return om_node;
}

static axutil_array_list_t *
process_subscriber_list_node(
    const axutil_env_t *env,
    axiom_node_t *subs_list_node)
{
    axiom_element_t *subs_list_element = NULL;
    axiom_children_qname_iterator_t *subs_iter = NULL;
    axutil_qname_t *qname = NULL;
    axutil_array_list_t *subscriber_list = NULL;
    axis2_status_t status = AXIS2_SUCCESS;

    AXIS2_LOG_TRACE(env->log, AXIS2_LOG_SI, 
        "[savan] Entry:process_subscriber_list_node");
    subs_list_element = axiom_node_get_data_element(subs_list_node, env); 
         
    /* Get Subscriber elements from subscriber list */
    qname = axutil_qname_create(env, ELEM_NAME_SUBSCRIBER, SAVAN_NAMESPACE, 
        NULL);
    subs_iter = axiom_element_get_children_with_qname(subs_list_element, env,
        qname, subs_list_node);
    axutil_qname_free(qname, env);
    if(!subs_iter)
    {
        AXIS2_LOG_DEBUG(env->log, AXIS2_LOG_SI, "[savan] Subscribers list is empty");
        return NULL;
    }

    if(axiom_children_qname_iterator_has_next(subs_iter, env))
    {
        subscriber_list = axutil_array_list_create(env, 0);
    }

    while(axiom_children_qname_iterator_has_next(subs_iter, env))
    {
        savan_subscriber_t *subscriber = NULL;
        axiom_node_t *subs_node = NULL;
     
        subs_node = axiom_children_qname_iterator_next(subs_iter, env);
        if(subs_node) /* Iterate Savan specific subscriber elements */
        {
            /* Now read Savan specific Subscribe element */
            subscriber = savan_util_process_savan_specific_subscriber_node(env, subs_node);
            if(!subscriber)
            {
                AXIS2_LOG_ERROR(env->log, AXIS2_LOG_SI, 
                        "[savan] Failed process Savan specific Subscriber element");
                status = axutil_error_get_status_code(env->error);
                return NULL;

            }

            axutil_array_list_add(subscriber_list, env, subscriber);
        }
    }

    AXIS2_LOG_TRACE(env->log, AXIS2_LOG_SI, "[savan] Exit:process_subscriber_list_node");
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

    if(endto_ref)
    {
        endto = axis2_endpoint_ref_get_address(endto_ref, env);
    }

    notify_ref = savan_subscriber_get_notify_to(subscriber, env);
    if(notify_ref)
    {
        notify = axis2_endpoint_ref_get_address(notify_ref, env);
    }

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
    {
        axiom_element_set_text(topic_elem, env, topic, topic_node);
    }

    /* create the subscriber element */
    sub_elem = axiom_element_create(env, add_node, ELEM_NAME_SUBSCRIBE, ns, &sub_node);
    
    /* EndTo element */
    endto_elem = axiom_element_create(env, sub_node, ELEM_NAME_ENDTO, ns, &endto_node);
    axiom_element_set_text(endto_elem, env, endto, endto_node);
    
    /* Delivery element */
    delivery_elem = axiom_element_create(env, sub_node, ELEM_NAME_DELIVERY, ns, &delivery_node);
        
    notify_elem = axiom_element_create(env, delivery_node, ELEM_NAME_NOTIFYTO, ns, &notify_node);
    axiom_element_set_text(notify_elem, env, notify, notify_node);
    
    /* Expires element */
    expires_elem = axiom_element_create(env, sub_node, ELEM_NAME_EXPIRES, ns, &expires_node);
    axiom_element_set_text(expires_elem, env, expires, expires_node);

    /* Filter element */
    filter_elem = axiom_element_create(env, sub_node, ELEM_NAME_FILTER, ns, &endto_node);
    axiom_element_set_text(filter_elem, env, filter, filter_node);
    
    return add_node;
}

/*static axiom_node_t *
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
    
    // create the id element
    if(id)
    {
        id_elem = axiom_element_create(env, remove_node, ELEM_NAME_ID, ns1, &id_node);
            axiom_element_set_text(id_elem, env, id, id_node);
    }
    // create the topic element
    topic_elem = axiom_element_create(env, remove_node, ELEM_NAME_TOPIC, ns1, &topic_node);
    topic = savan_subscriber_get_topic(subscriber, env);
    if(topic)
        axiom_element_set_text(topic_elem, env, topic, topic_node);
    
    return remove_node;
}*/

axis2_char_t * AXIS2_CALL
savan_util_get_expiry_time(
    const axutil_env_t *env)
{
    /* TODO: decide how to set expiry time */
    
    return NULL;
}

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
    axis2_char_t *temp = NULL;

    temp = axutil_rindex(topic_url, '/') + 1;
    if(temp)
    {
        topic  = axutil_strdup(env, temp);
    }

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

    AXIS2_LOG_TRACE(env->log, AXIS2_LOG_SI, 
        "[savan] Entry:savan_util_get_topic_list_from_remote_subs_mgr");

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
            "[savan] Stub invoke FAILED: Error code:"
            " %d :: %s", env->error->error_number,
            AXIS2_ERROR_GET_MESSAGE(env->error));
    }
    if(!s_client && svc_client)
    {
        /*axis2_svc_client_free(svc_client, env);*/
    }
    AXIS2_LOG_TRACE(env->log, AXIS2_LOG_SI, 
        "[savan] Exit:savan_util_get_topic_list_from_remote_subs_mgr");
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
            "[savan] Sending topics_request_om_payload: %s", om_str);
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

    AXIS2_LOG_TRACE(env->log, AXIS2_LOG_SI, "[savan] Entry:process_topic_list_node");

    topic_list_element = axiom_node_get_data_element(topic_list_node, env); 
         
    /* Get topic elements from topic list */
    qname = axutil_qname_create(env, ELEM_NAME_TOPIC, SAVAN_NAMESPACE, NULL);
    topic_iter = axiom_element_get_children_with_qname(topic_list_element, env, qname, 
            topic_list_node);

    axutil_qname_free(qname, env);
    if(!topic_iter)
    {
        AXIS2_LOG_DEBUG(env->log, AXIS2_LOG_SI, "[savan] Topic list is empty");
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
            axutil_array_list_add(topic_list, env, axutil_strdup(env, topic_url_str));
            AXIS2_LOG_DEBUG(env->log, AXIS2_LOG_SI, "topic_url_str:%s", topic_url_str);
        }
    }

    AXIS2_LOG_TRACE(env->log, AXIS2_LOG_SI, "[savan] Exit:process_topic_list_node");
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
    {
        client_home = "../../deploy";
    }

    options = axis2_options_create(env);
    axis2_options_set_xml_parser_reset(options, env, AXIS2_FALSE);
    svc_client = axis2_svc_client_create(env, client_home);
    if (!svc_client)
    {
        AXIS2_LOG_ERROR(env->log, AXIS2_LOG_SI, 
            "[savan] Stub invoke FAILED: Error code:"
            " %d :: %s", env->error->error_number,
            AXIS2_ERROR_GET_MESSAGE(env->error));
        return NULL;
    }
    axis2_svc_client_set_options(svc_client, env, options);    
    axis2_svc_client_engage_module(svc_client, env, AXIS2_MODULE_ADDRESSING);
    return svc_client;
}

AXIS2_EXTERN axis2_char_t *AXIS2_CALL
savan_util_get_dbname(
    const axutil_env_t *env,
    axis2_conf_t *conf)
{
    axis2_char_t *path = "./savan_db";
    axis2_module_desc_t *module_desc = NULL;
    axutil_qname_t *qname = NULL;

    qname = axutil_qname_create(env, SAVAN_MODULE, NULL, NULL);
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
    
    return path;
}

AXIS2_EXTERN axis2_char_t *AXIS2_CALL
savan_util_get_module_param(
    const axutil_env_t *env,
    axis2_conf_t *conf,
    axis2_char_t *name)
{
    axis2_char_t *value = NULL;
    axis2_module_desc_t *module_desc = NULL;
    axutil_qname_t *qname = NULL;

    qname = axutil_qname_create(env, SAVAN_MODULE, NULL, NULL);
    module_desc = axis2_conf_get_module(conf, env, qname);
    if(module_desc)
    {
        axutil_param_t *param = NULL;
        param = axis2_module_desc_get_param(module_desc, env, name);
        if(param)
        {
            value = (axis2_char_t *) axutil_param_get_value(param, env);
        }
    }
    axutil_qname_free(qname, env);
    
    return value;
}

AXIS2_EXTERN axis2_status_t AXIS2_CALL
savan_util_process_subscriber_node(
    const axutil_env_t *env,
    axiom_node_t *sub_node,
    axiom_element_t *sub_elem,
    savan_subscriber_t *subscriber)
{
    axutil_qname_t *qname = NULL;
    axiom_node_t *endto_node = NULL;
    axiom_node_t *delivery_node = NULL;
    axiom_node_t *notify_node = NULL;
    axiom_node_t *filter_node = NULL;
    axiom_node_t *expires_node = NULL;
    
    axiom_element_t *endto_elem = NULL;
    axiom_element_t *delivery_elem = NULL;
    axiom_element_t *notify_elem = NULL;
    axiom_element_t *expires_elem = NULL;
    axiom_element_t *filter_elem = NULL;
    
    axis2_char_t *endto = NULL;
    axis2_char_t *notify = NULL;
    axis2_char_t *expires = NULL;
    axis2_char_t *filter = NULL;
    axis2_char_t *filter_dialect = NULL;
    
    axis2_endpoint_ref_t *endto_epr = NULL;
    axis2_endpoint_ref_t *topic_epr = NULL;
    axis2_endpoint_ref_t *notify_epr = NULL;

    axis2_status_t status = AXIS2_SUCCESS;

    AXIS2_LOG_TRACE(env->log, AXIS2_LOG_SI, "[savan] Entry:savan_util_process_subscriber_node");

    if(sub_elem)
    {
        /* EndTo */
        qname = axutil_qname_create(env, ELEM_NAME_ENDTO, EVENTING_NAMESPACE, NULL);
        endto_elem = axiom_element_get_first_child_with_qname(sub_elem, env, qname, sub_node, 
                &endto_node);
        axutil_qname_free(qname, env);
       
        if(endto_elem)
        {
            endto = axiom_element_get_text(endto_elem, env, endto_node);
            if(endto)
            {
                endto_epr = axis2_endpoint_ref_create(env, endto);
                savan_subscriber_set_end_to(subscriber, env, endto_epr);
            }
        }
        
        /* Get Delivery element and read NotifyTo */
        qname = axutil_qname_create(env, ELEM_NAME_DELIVERY, EVENTING_NAMESPACE, NULL);
        delivery_elem = axiom_element_get_first_child_with_qname(sub_elem, env, qname, sub_node, 
                &delivery_node);

        axutil_qname_free(qname, env);
        if(delivery_elem)
        {
            qname = axutil_qname_create(env, ELEM_NAME_NOTIFYTO, EVENTING_NAMESPACE, NULL);
            notify_elem = axiom_element_get_first_child_with_qname(delivery_elem, env, qname,
                                                                   delivery_node, &notify_node);
            axutil_qname_free(qname, env);
            if(notify_elem)
            {
                notify = axiom_element_get_text(notify_elem, env, notify_node);
                if(notify)
                {
                    notify_epr = axis2_endpoint_ref_create(env, notify);
                    savan_subscriber_set_notify_to(subscriber, env, notify_epr);
                }
            }
        }

        /* Expires */
        qname = axutil_qname_create(env, ELEM_NAME_EXPIRES, EVENTING_NAMESPACE, NULL);
        expires_elem = axiom_element_get_first_child_with_qname(sub_elem, env, qname,
                                                                sub_node, &expires_node);
        axutil_qname_free(qname, env);
        if(expires_elem)
        {
            expires = axiom_element_get_text(expires_elem, env, expires_node);
            if(expires)
            {
                savan_subscriber_set_expires(subscriber, env, expires);
            }
        }
        
        /* Filter */
        qname = axutil_qname_create(env, ELEM_NAME_FILTER, EVENTING_NAMESPACE, NULL);
        filter_elem = axiom_element_get_first_child_with_qname(sub_elem, env, 
                                                               qname,
                                                               sub_node, 
                                                               &filter_node);
        axutil_qname_free(qname, env);
        if(filter_elem)
        {
            qname = axutil_qname_create(env, SAVAN_FILTER_DIALECT, NULL, NULL);
            filter = axiom_element_get_text(filter_elem, env, filter_node);
            filter_dialect = axiom_element_get_attribute_value(filter_elem,
                                                               env, qname);
            axutil_qname_free(qname, env);
            if(filter_dialect)
            {
                savan_subscriber_set_filter_dialect(subscriber, env, 
                                                    filter_dialect);
            }

            if(filter)
            {
                savan_subscriber_set_filter(subscriber, env, filter);
            }
        }

        if(endto)
        {
            topic_epr = axis2_endpoint_ref_create(env, endto);
        }

        if(topic_epr)
        {
            axis2_char_t *topic = NULL;
            topic = (axis2_char_t *) axis2_endpoint_ref_get_address(topic_epr, env);
            printf("topic:%s\n",  topic);
            if(topic)
            {
                savan_subscriber_set_topic(subscriber, env, topic);
            }
        }
    }

    AXIS2_LOG_TRACE(env->log, AXIS2_LOG_SI, "[savan] Exit:savan_util_process_subscriber_node");
    return status;
}

AXIS2_EXTERN axiom_node_t * AXIS2_CALL
savan_util_create_subscriber_node(
    const axutil_env_t *env,
    savan_subscriber_t *subscriber,
    axiom_node_t *parent_node)
{
	axiom_attribute_t *dialect = NULL;
    axiom_namespace_t *ns = NULL;
    axiom_node_t *sub_node = NULL;
    axiom_node_t *endto_node = NULL;
    axiom_node_t *delivery_node = NULL;
    axiom_node_t *notify_node = NULL;
    axiom_node_t *filter_node = NULL;
    axiom_node_t *expires_node = NULL;
    axiom_element_t* sub_elem = NULL;
    axiom_element_t* endto_elem = NULL;
    axiom_element_t* delivery_elem = NULL;
    axiom_element_t* notify_elem = NULL;
    axiom_element_t* filter_elem = NULL;
    axiom_element_t* expires_elem = NULL;
    axis2_char_t *endto = NULL;
    axis2_char_t *notify = NULL;
    axis2_char_t *filter = NULL;
    axis2_char_t *filter_dialect = NULL;
    axis2_char_t *expires = NULL;
    axis2_endpoint_ref_t *endpoint_ref = NULL;

    AXIS2_LOG_TRACE(env->log, AXIS2_LOG_SI, "[savan] Entry:savan_util_create_subscriber_node");
    if(!subscriber)
    {
        axutil_error_set_status_code(env->error, AXIS2_FAILURE);
        AXIS2_LOG_ERROR(env->log, AXIS2_LOG_SI, "[savan] Subscriber structure passed is NULL");
        return NULL;
    }

    endpoint_ref = savan_subscriber_get_end_to(subscriber, env);
    endto = (axis2_char_t *) axis2_endpoint_ref_get_address(endpoint_ref, env);
    endpoint_ref = savan_subscriber_get_notify_to(subscriber, env);
    notify = (axis2_char_t *) axis2_endpoint_ref_get_address(endpoint_ref, env);
    filter = savan_subscriber_get_filter(subscriber, env);
    filter_dialect = savan_subscriber_get_filter_dialect(subscriber, env);
    expires = savan_subscriber_get_expires(subscriber, env);

    /* create the body of the Subscribe request */
    ns = axiom_namespace_create (env, EVENTING_NAMESPACE, EVENTING_NS_PREFIX);
    sub_elem = axiom_element_create(env, parent_node, ELEM_NAME_SUBSCRIBE, ns, &sub_node);
    
    /* EndTo element */
    endto_elem = axiom_element_create(env, sub_node, ELEM_NAME_ENDTO, ns, &endto_node);
    axiom_element_set_text(endto_elem, env, endto, endto_node);
    
    /* Delivery element */
    delivery_elem = axiom_element_create(env, sub_node, ELEM_NAME_DELIVERY, ns, &delivery_node);
        
    notify_elem = axiom_element_create(env, delivery_node, ELEM_NAME_NOTIFYTO, ns, &notify_node);
    axiom_element_set_text(notify_elem, env, notify, notify_node);
    
    /* Expires element */
    expires_elem = axiom_element_create(env, sub_node, ELEM_NAME_EXPIRES, ns, &expires_node);
    axiom_element_set_text(expires_elem, env, expires, expires_node);
    /* Filter element */
    filter_elem = axiom_element_create(env, sub_node, ELEM_NAME_FILTER, ns, &filter_node);
    axiom_element_set_text(filter_elem, env, filter, filter_node);

	if(!filter_dialect) 
	{
		dialect = axiom_attribute_create(env, "Dialect", DEFAULT_FILTER_DIALECT, NULL);
	}
	else
	{
		dialect = axiom_attribute_create(env, "Dialect", filter_dialect, NULL);
	}

	axiom_element_add_attribute(filter_elem, env, dialect ,filter_node);

    AXIS2_LOG_TRACE(env->log, AXIS2_LOG_SI, "[savan] Exit:savan_util_create_subscriber_node");
    return sub_node;
}

AXIS2_EXTERN axiom_node_t * AXIS2_CALL
savan_util_create_savan_specific_subscriber_node(
    const axutil_env_t *env, 
    savan_subscriber_t *subscriber,
    axiom_node_t *parent_node)
{
    axiom_node_t *subs_node = NULL;
    axis2_status_t status = AXIS2_FAILURE;
    axiom_namespace_t *ns1 = NULL;
    axiom_namespace_t *ns2 = NULL;
    axiom_node_t *sub_node = NULL;
    axiom_node_t *id_node = NULL;
    axiom_element_t *subs_elem = NULL;
    axiom_element_t* id_elem = NULL;
    axis2_char_t *id = NULL;

    AXIS2_LOG_TRACE(env->log, AXIS2_LOG_SI, 
            "[savan] Entry:savan_util_create_savan_specific_subscriber_node");

    if(!subscriber)
    {
        AXIS2_LOG_ERROR(env->log, AXIS2_LOG_SI, "[savan] Subscriber structure must be present");
        axutil_error_set_status_code(env->error, AXIS2_FAILURE);
        return NULL;
    }

    ns1 = axiom_namespace_create (env, SAVAN_NAMESPACE, SAVAN_NS_PREFIX);
    subs_elem = axiom_element_create(env, parent_node, ELEM_NAME_SUBSCRIBER, ns1, &subs_node);
    if(!subs_node)
    {
        AXIS2_LOG_ERROR(env->log, AXIS2_LOG_SI, 
                "[savan] Could not create Savan specific subscriber node");
        status = axutil_error_get_status_code(env->error);
        if(AXIS2_SUCCESS != status)
        {
            return NULL;
        }
    }

    /* Id element */
    id = savan_subscriber_get_id(subscriber, env);
    ns2 = axiom_namespace_create (env, SAVAN_NAMESPACE, SAVAN_NS_PREFIX);
    id_elem = axiom_element_create(env, subs_node, ELEM_NAME_ID, ns2, &id_node);
    axiom_element_set_text(id_elem, env, id, id_node);

    sub_node = savan_util_create_subscriber_node(env, subscriber, subs_node);
    if(!sub_node)
    {
        AXIS2_LOG_ERROR(env->log, AXIS2_LOG_SI, "[savan] Creating subscriber node failed");
        return NULL;
    }

    AXIS2_LOG_TRACE(env->log, AXIS2_LOG_SI, 
            "[savan] Exit:savan_util_create_savan_specific_subscriber_node");
    return subs_node;
}

