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

#include <axiom_xml_writer.h>
#include <axiom_soap_envelope.h>
#include <axiom_soap_body.h>
#include <axis2_svc.h>
#include <stdio.h>
#include <savan_subscriber.h>
#include <savan_util.h>
#include <savan_constants.h>

#include "savan_subs_mgr.h"
#include <savan_db_mgr.h>
#include <savan_util.h>

AXIS2_EXTERN axiom_node_t *AXIS2_CALL
savan_subs_mgr_add_subscriber(
    const axutil_env_t *env,
    axiom_node_t *add_sub_node,
    axis2_msg_ctx_t *msg_ctx)
{
    axis2_conf_ctx_t *conf_ctx = NULL;
    axis2_conf_t *conf = NULL;
    savan_subscriber_t *subscriber = NULL;
    axis2_char_t *topic_url = NULL;
    axis2_char_t *topic = NULL;

    axutil_qname_t *qname = NULL;
    
    axiom_node_t *sub_node = NULL;
    axiom_node_t *id_node = NULL;
    axiom_node_t *topic_node = NULL;
    axiom_node_t *endto_node = NULL;
    axiom_node_t *delivery_node = NULL;
    axiom_node_t *notify_node = NULL;
    axiom_node_t *filter_node = NULL;
    axiom_node_t *expires_node = NULL;
    
    axiom_element_t *sub_elem = NULL;
    axiom_element_t *id_elem = NULL;
    axiom_element_t *topic_elem = NULL;
    axiom_element_t *add_sub_elem = NULL;
    axiom_element_t *endto_elem = NULL;
    axiom_element_t *delivery_elem = NULL;
    axiom_element_t *notify_elem = NULL;
    axiom_element_t *expires_elem = NULL;
    axiom_element_t *filter_elem = NULL;
    
    axis2_char_t *id = NULL;
    axis2_char_t *endto = NULL;
    axis2_char_t *notifyto = NULL;
    axis2_char_t *expires = NULL;
    axis2_char_t *filter = NULL;

    AXIS2_LOG_DEBUG(env->log, AXIS2_LOG_SI, 
        "[savan] Start:savan_subs_mgr_add_subscriber");
    conf_ctx = axis2_msg_ctx_get_conf_ctx(msg_ctx, env);
    conf = axis2_conf_ctx_get_conf(conf_ctx, env);
    subscriber = savan_subscriber_create(env);
    if(add_sub_node)
        add_sub_elem = (axiom_element_t*)axiom_node_get_data_element(
            add_sub_node, env);
    
    AXIS2_LOG_DEBUG(env->log, AXIS2_LOG_SI, 
        "[savan] node:%s", axiom_node_to_string(add_sub_node, env));
    /*Get topic element from node */
    qname = axutil_qname_create(env, ELEM_NAME_TOPIC, SAVAN_NAMESPACE, NULL);
    topic_elem = axiom_element_get_first_child_with_qname(add_sub_elem, env, qname,
        add_sub_node, &topic_node);
    axutil_qname_free(qname, env);
    topic_url = axiom_element_get_text(topic_elem, env, topic_node);
    if(topic_url)
    {
        savan_subscriber_set_topic(subscriber, env, topic_url);
        AXIS2_LOG_DEBUG(env->log, AXIS2_LOG_SI, 
            "[savan] Subscriber will be added to the topic:%s ", topic_url);
        topic = savan_util_get_topic_name_from_topic_url(env, topic_url);
    }

    /* Get Id element from AddSubscriber*/
    qname = axutil_qname_create(env, ELEM_NAME_ID, SAVAN_NAMESPACE, NULL);
    id_elem = axiom_element_get_first_child_with_qname(add_sub_elem, env, qname,
        add_sub_node, &id_node);
    axutil_qname_free(qname, env);
  
    if(id_elem)
    {
        id = axiom_element_get_text(id_elem, env, id_node);
        savan_subscriber_set_id(subscriber, env, id);
    }
    /* Get subscriber element from Body */
    qname = axutil_qname_create(env, ELEM_NAME_SUBSCRIBE, EVENTING_NAMESPACE, NULL);
    sub_elem = axiom_element_get_first_child_with_qname(add_sub_elem, env, qname,
        add_sub_node, &sub_node);
    axutil_qname_free(qname, env);
    
    /* Now read each sub element of Subscribe element */
        
    /* EndTo */
    if(sub_elem)
    {
        qname = axutil_qname_create(env, ELEM_NAME_ENDTO, EVENTING_NAMESPACE, NULL);
        endto_elem = axiom_element_get_first_child_with_qname(sub_elem, env, qname,
            sub_node, &endto_node);
        axutil_qname_free(qname, env);
        if(endto_elem)
        {
            axis2_endpoint_ref_t *endto_ref = NULL;
            endto = axiom_element_get_text(endto_elem, env, endto_node);
            if(endto)
            {
                endto_ref = axis2_endpoint_ref_create(env, endto);
                savan_subscriber_set_end_to(subscriber, env, endto_ref);
            }
        }
    
        /* Get Delivery element and read NotifyTo */
        qname = axutil_qname_create(env, ELEM_NAME_DELIVERY, EVENTING_NAMESPACE, 
            NULL);
        delivery_elem = axiom_element_get_first_child_with_qname(sub_elem, env, 
            qname, sub_node, &delivery_node);
        axutil_qname_free(qname, env);
   
        if(delivery_elem)
        {
            qname = axutil_qname_create(env, ELEM_NAME_NOTIFYTO, 
                EVENTING_NAMESPACE, NULL);
            notify_elem = axiom_element_get_first_child_with_qname(
                delivery_elem, env, qname, delivery_node, &notify_node);
            axutil_qname_free(qname, env);
            if(notify_elem)
            {
                axis2_endpoint_ref_t *notifyto_ref = NULL;
                notifyto = axiom_element_get_text(notify_elem, env, notify_node);
                if(notifyto)
                {
                    notifyto_ref = axis2_endpoint_ref_create(env, notifyto);
                    savan_subscriber_set_notify_to(subscriber, env, notifyto_ref);
                }
            }
        }
    
        /* Expires */
        qname = axutil_qname_create(env, ELEM_NAME_EXPIRES, EVENTING_NAMESPACE, NULL);
        expires_elem = axiom_element_get_first_child_with_qname(sub_elem, env, qname, sub_node, 
                &expires_node);

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
        filter_elem = axiom_element_get_first_child_with_qname(sub_elem, env, qname, sub_node, 
                &filter_node);

        axutil_qname_free(qname, env);
        if(filter_elem)
        {
            filter = axiom_element_get_text(filter_elem, env, filter_node);
            if(filter)
            {
                savan_subscriber_set_filter(subscriber, env, filter);
            }
        }
    }

    if(savan_db_mgr_insert_subscriber(env, savan_util_get_dbname(env, conf), subscriber))
    {
        AXIS2_LOG_DEBUG(env->log, AXIS2_LOG_SI, 
            "[savan] Subscriber %s added to the topic:%s", id, topic_url);
    }
    else
    {
        AXIS2_LOG_DEBUG(env->log, AXIS2_LOG_SI, 
                "[savan] Subscriber %s could not be added to the topic:%s", id, topic_url);
    }

    AXIS2_LOG_DEBUG(env->log, AXIS2_LOG_SI, "[savan] End:savan_subs_mgr_add_subscriber");
    return NULL;   
}

AXIS2_EXTERN void * AXIS2_CALL
savan_subs_mgr_remove_subscriber(
    const axutil_env_t *env,
    axiom_node_t *remove_sub_node,
    axis2_msg_ctx_t *msg_ctx)
{
    axis2_char_t *topic = NULL;
    axis2_char_t *topic_url = NULL;

    axutil_qname_t *qname = NULL;
    
    axiom_node_t *id_node = NULL;
    axiom_node_t *topic_node = NULL;
    
    axiom_element_t *id_elem = NULL;
    axiom_element_t *topic_elem = NULL;
    axiom_element_t *remove_sub_elem = NULL;
    
    axis2_char_t *id = NULL;
    axis2_char_t sql_remove[256];
    axis2_conf_ctx_t *conf_ctx = NULL;
    axis2_conf_t *conf = NULL;

    AXIS2_LOG_DEBUG(env->log, AXIS2_LOG_SI, "[savan] Start:savan_subs_mgr_remove_subscriber");

    conf_ctx = axis2_msg_ctx_get_conf_ctx(msg_ctx, env);
    conf = axis2_conf_ctx_get_conf(conf_ctx, env);
    remove_sub_elem = (axiom_element_t*)axiom_node_get_data_element(
        remove_sub_node, env);
    
    AXIS2_LOG_DEBUG(env->log, AXIS2_LOG_SI, "[savan] node:%s", 
        axiom_node_to_string(remove_sub_node, env));
    /*Get topic element from node */
    qname = axutil_qname_create(env, ELEM_NAME_TOPIC, SAVAN_NAMESPACE, NULL);
    topic_elem = axiom_element_get_first_child_with_qname(remove_sub_elem, env, 
        qname, remove_sub_node, &topic_node);
    axutil_qname_free(qname, env);
    topic_url = axiom_element_get_text(topic_elem, env, topic_node);
    topic = savan_util_get_topic_name_from_topic_url(env, topic_url);
    AXIS2_LOG_DEBUG(env->log, AXIS2_LOG_SI, 
        "[savan] Subscriber will be removed from the topic:%s ", topic);

    /* Get Id element from RemoveSubscriber*/
    qname = axutil_qname_create(env, ELEM_NAME_ID, SAVAN_NAMESPACE, NULL);
    id_elem = axiom_element_get_first_child_with_qname(remove_sub_elem, env, 
        qname, remove_sub_node, &id_node);
    axutil_qname_free(qname, env);
    
    id = axiom_element_get_text(id_elem, env, id_node);
    
    AXIS2_LOG_DEBUG(env->log, AXIS2_LOG_SI, 
        "[savan] Removing subscriber with id %s from topic %s", id, topic_url);
    
    sprintf(sql_remove, "delete from subscriber where id='%s'", id);

    savan_db_mgr_remove(env, savan_util_get_dbname(env, conf), sql_remove);

    AXIS2_LOG_DEBUG(env->log, AXIS2_LOG_SI, 
        "[savan] Subscriber %s removed from the topic:%s", id, topic_url);

    AXIS2_LOG_DEBUG(env->log, AXIS2_LOG_SI, 
        "[savan] End:savan_subs_mgr_remove_subscriber");

    return NULL;   
}

AXIS2_EXTERN axiom_node_t *AXIS2_CALL
savan_subs_mgr_get_subscriber(
    const axutil_env_t *env,
    axiom_node_t *node,
    axis2_msg_ctx_t *msg_ctx)
{
    axiom_namespace_t *ns = NULL;
    axiom_namespace_t *ns1 = NULL;
    axiom_node_t *subs_id_parent_node = NULL;
    axiom_node_t *subs_id_node = NULL;
    savan_subscriber_t *subscriber = NULL;
    axis2_char_t *subs_id = NULL;
    axiom_node_t *subs_node = NULL;
    axiom_element_t *subs_elem = NULL;
    axis2_conf_ctx_t *conf_ctx = NULL;
    axis2_conf_t *conf = NULL;
    axis2_char_t sql_retrieve[256];

    AXIS2_LOG_DEBUG(env->log, AXIS2_LOG_SI, 
        "Start:savan_subs_mgr_get_subscriber");

    /* Expected request format is :-
     * <ns1:get_subscriber xmlns:ns1="http://ws.apache.org/savan">
     *      <SubscriberId>subscriber id</SubscriberId>
     * </ns1:get_subscriber>
     */
    conf_ctx = axis2_msg_ctx_get_conf_ctx(msg_ctx, env);
    conf = axis2_conf_ctx_get_conf(conf_ctx, env);
    if (!node) /* 'get_subscriber' node */
    {
        AXIS2_ERROR_SET(env->error, AXIS2_ERROR_SVC_SKEL_INPUT_OM_NODE_NULL, 
            AXIS2_FAILURE);
        AXIS2_LOG_ERROR(env->log, AXIS2_LOG_SI, 
            "Echo client ERROR: input parameter NULL");
        return NULL;
    }

    subs_id_parent_node = axiom_node_get_first_element(node, env);
    if (!subs_id_parent_node) 
    {
        AXIS2_ERROR_SET(env->error, 
            AXIS2_ERROR_SVC_SKEL_INVALID_XML_FORMAT_IN_REQUEST, AXIS2_FAILURE);
        AXIS2_LOG_ERROR(env->log, AXIS2_LOG_SI, 
            "Echo client ERROR 1: invalid XML in request");
        return NULL;
    }

    subs_id_node = axiom_node_get_first_child(subs_id_parent_node, env);
    if (!subs_id_node) /* actual subs_id text */
    {
        AXIS2_ERROR_SET(env->error, 
            AXIS2_ERROR_SVC_SKEL_INVALID_XML_FORMAT_IN_REQUEST, AXIS2_FAILURE);
        AXIS2_LOG_ERROR(env->log, AXIS2_LOG_SI, "invalid XML in request");
        return NULL;
    }

    if (axiom_node_get_node_type(subs_id_node, env) == AXIOM_TEXT)
    {
        axiom_text_t *subs_id_text = 
            (axiom_text_t *)axiom_node_get_data_element(subs_id_node, env);
        if (subs_id_text && axiom_text_get_value(subs_id_text , env))
        {
            subs_id = (axis2_char_t *)axiom_text_get_value(subs_id_text, env);
            AXIS2_LOG_DEBUG(env->log, AXIS2_LOG_SI, 
                "Requested Subscriber's id:%s", subs_id);
        }
    }
    else
    {
        AXIS2_ERROR_SET(env->error, 
            AXIS2_ERROR_SVC_SKEL_INVALID_XML_FORMAT_IN_REQUEST, AXIS2_FAILURE);
        AXIS2_LOG_ERROR(env->log, AXIS2_LOG_SI, "Invalid XML in request");
        return NULL;
    }

    /* create the body of the subscribers element */
    ns = axiom_namespace_create (env, EVENTING_NAMESPACE, EVENTING_NS_PREFIX);
    ns1 = axiom_namespace_create (env, SAVAN_NAMESPACE, SAVAN_NS_PREFIX);
    subs_elem = axiom_element_create(env, NULL, ELEM_NAME_SUBSCRIBER, ns1, 
        &subs_node);
    sprintf(sql_retrieve, "select id, end_to, notify_to, delivery_mode, "\
        "expires, filter, renewed, topic_url from subscriber, topic"\
        " where id='%s' and topic.topic_name=subscriber.topic_name;", subs_id);

    subscriber = savan_db_mgr_retrieve(env, savan_util_get_dbname(env, conf), 
                savan_db_mgr_subs_retrieve_callback, sql_retrieve);

    if (subscriber)
    {
        axiom_node_t *sub_node = NULL;
        axiom_node_t *endto_node = NULL;
        axiom_node_t *id_node = NULL;
        axiom_node_t *delivery_node = NULL;
        axiom_node_t *notify_node = NULL;
        axiom_node_t *filter_node = NULL;
        axiom_node_t *expires_node = NULL;
        axiom_element_t* sub_elem = NULL;
        axiom_element_t* id_elem = NULL;
        axiom_element_t* endto_elem = NULL;
        axiom_element_t* delivery_elem = NULL;
        axiom_element_t* notify_elem = NULL;
        axiom_element_t* filter_elem = NULL;
        axiom_element_t* expires_elem = NULL;
        const axis2_char_t *endto = NULL;
        const axis2_char_t *notify = NULL;
        const axis2_char_t *filter = NULL;
        const axis2_char_t *expires = NULL;
        axis2_char_t *id = NULL;
		axis2_endpoint_ref_t *endto_ref = NULL;
		axis2_endpoint_ref_t *notify_ref = NULL;

        endto_ref = savan_subscriber_get_end_to(subscriber, env);
        endto = axis2_endpoint_ref_get_address(endto_ref, env);
        notify_ref = savan_subscriber_get_notify_to(subscriber, env);
        notify = axis2_endpoint_ref_get_address(notify_ref, env);
        filter = savan_subscriber_get_filter(subscriber, env); 
        expires = savan_subscriber_get_expires(subscriber, env);
        id = savan_subscriber_get_id(subscriber, env);
      

        /* create the subscriber element */

        sub_elem = axiom_element_create(env, subs_node, 
            ELEM_NAME_SUBSCRIBE, ns, &sub_node);
        
        /* Id element */
        id_elem = axiom_element_create(env, sub_node, ELEM_NAME_ID, ns1,
            &id_node);
        axiom_element_set_text(id_elem, env, id, id_node);

        /* EndTo element */
        endto_elem = axiom_element_create(env, sub_node, ELEM_NAME_ENDTO, ns,
            &endto_node);
        axiom_element_set_text(endto_elem, env, endto, endto_node);
        
        /* Delivery element */
        delivery_elem = axiom_element_create(env, sub_node, 
            ELEM_NAME_DELIVERY, ns, &delivery_node);
            
        notify_elem = axiom_element_create(env, delivery_node, 
            ELEM_NAME_NOTIFYTO, ns, &notify_node);
        axiom_element_set_text(notify_elem, env, notify, notify_node);
        
        /* Expires element */
        expires_elem = axiom_element_create(env, sub_node, 
            ELEM_NAME_EXPIRES, ns, &expires_node);
        axiom_element_set_text(expires_elem, env, expires, expires_node);
        /* Filter element */
        filter_elem = axiom_element_create(env, sub_node, ELEM_NAME_FILTER, 
            ns, &endto_node);
        axiom_element_set_text(filter_elem, env, filter, filter_node);
    }
    AXIS2_LOG_DEBUG(env->log, AXIS2_LOG_SI, 
        "End:savan_subs_mgr_get_subscriber");
    return subs_node;
}

AXIS2_EXTERN axiom_node_t *AXIS2_CALL
savan_subs_mgr_get_subscriber_list(
    const axutil_env_t *env,
    axiom_node_t *node,
    axis2_msg_ctx_t *msg_ctx)
{
    axis2_char_t *topic_url = NULL;
    axis2_char_t *topic = NULL;
    axutil_array_list_t *subs_store = NULL;
    axiom_namespace_t *ns = NULL;
    axiom_namespace_t *ns1 = NULL;
    axiom_node_t *subs_list_node = NULL;
    axiom_node_t *topic_parent_node = NULL;
    axiom_node_t *topic_node = NULL;
    axiom_element_t* subs_list_elem = NULL;
    axis2_conf_ctx_t *conf_ctx = NULL;
    axis2_conf_t *conf = NULL;
    axis2_char_t sql_retrieve[256];
    int i = 0, size = 0;

    AXIS2_LOG_DEBUG(env->log, AXIS2_LOG_SI, 
        "Start:savan_subs_mgr_get_subscriber_list");
    /* Expected request format is :-
     * <ns1:get_subscriber_list xmlns:ns1="http://ws.apache.org/savan">
     *      <Topic>topic_url</Topic>
     *      <ns:Susbscriber xmlns:ns="http://schemas.xmlsoap.org/ws/2004/08/eventing">
     *      ...
     *      </ns:Subscriber>
     *      <ns:Susbscriber xmlns:ns="http://schemas.xmlsoap.org/ws/2004/08/eventing">
     *      ...
     *      </ns:get_subscriber_list>
     * </ns1:Subscribers>
     */
    conf_ctx = axis2_msg_ctx_get_conf_ctx(msg_ctx, env);
    conf = axis2_conf_ctx_get_conf(conf_ctx, env);
    if (!node) /* 'get_subscriber_list' node */
    {
        AXIS2_ERROR_SET(env->error, AXIS2_ERROR_SVC_SKEL_INPUT_OM_NODE_NULL, 
            AXIS2_FAILURE);
        AXIS2_LOG_ERROR(env->log, AXIS2_LOG_SI, 
                        "[savan]axiom node is null, unable to continue");
        return NULL;
    }

    topic_parent_node = axiom_node_get_first_element(node, env);
    if (!topic_parent_node) 
    {
        AXIS2_ERROR_SET(env->error, 
            AXIS2_ERROR_SVC_SKEL_INVALID_XML_FORMAT_IN_REQUEST, AXIS2_FAILURE);
        AXIS2_LOG_ERROR(env->log, AXIS2_LOG_SI, 
                        "[savan]topic parent node is null, unable to continue");
        return NULL;
    }

    topic_node = axiom_node_get_first_child(topic_parent_node, env);
    if (!topic_node) /* actual topic text */
    {
        AXIS2_ERROR_SET(env->error, 
            AXIS2_ERROR_SVC_SKEL_INVALID_XML_FORMAT_IN_REQUEST, AXIS2_FAILURE);
        AXIS2_LOG_ERROR(env->log, AXIS2_LOG_SI, "[savan]invalid XML in request");
        return NULL;
    }

    if (axiom_node_get_node_type(topic_node, env) == AXIOM_TEXT)
    {
        axiom_text_t *topic_text = (axiom_text_t *)axiom_node_get_data_element(
            topic_node, env);
        if (topic_text && axiom_text_get_value(topic_text , env))
        {
            topic_url = (axis2_char_t *)axiom_text_get_value(topic_text, env);
            topic = savan_util_get_topic_name_from_topic_url(env, topic_url);
            AXIS2_LOG_DEBUG(env->log, AXIS2_LOG_SI, "[savan]Requested Topic:%s", topic);
        }
    }
    else
    {
        AXIS2_ERROR_SET(env->error, 
            AXIS2_ERROR_SVC_SKEL_INVALID_XML_FORMAT_IN_REQUEST, AXIS2_FAILURE);
        AXIS2_LOG_ERROR(env->log, AXIS2_LOG_SI, "[savan]Invalid XML in request");
        return NULL;
    }

    sprintf(sql_retrieve, "select id, end_to, notify_to, delivery_mode, "\
        "expires, filter, topic_url, renewed from subscriber, topic where "\
        "subscriber.topic_name=topic.topic_name and topic.topic_name='%s';", 
            topic);

    subs_store = savan_db_mgr_retrieve_all(env, savan_util_get_dbname(env, conf),
            savan_db_mgr_subs_find_callback, sql_retrieve);

    if(subs_store)
    {
        size = axutil_array_list_size(subs_store, env);
        if(size > 0)
        {
            /* create the body of the subscribers element */
            ns = axiom_namespace_create (env, EVENTING_NAMESPACE, EVENTING_NS_PREFIX);
            ns1 = axiom_namespace_create (env, SAVAN_NAMESPACE, SAVAN_NS_PREFIX);
            subs_list_elem = axiom_element_create(env, NULL, ELEM_NAME_SUBSCRIBERS, ns1, 
            &subs_list_node);
        }
    }
    for(i = 0; i < size; i++)
    {
        savan_subscriber_t * subscriber = axutil_array_list_get(subs_store, 
            env, i);

        if (subscriber)
        {
            axiom_node_t *topic_node = NULL;
            axiom_node_t *sub_node = NULL;
            axiom_node_t *endto_node = NULL;
            axiom_node_t *id_node = NULL;
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
            const axis2_char_t *filter = NULL;
            const axis2_char_t *expires = NULL;
            axis2_char_t *id = NULL;
            axis2_char_t *topic_url_l = NULL;
            axis2_char_t *topic_l = NULL;
			axis2_endpoint_ref_t *endto_ref = NULL;
			axis2_endpoint_ref_t *notify_ref = NULL;

            /* Check whether the subscriber has subscribed for the topic. 
             * If so create the topic element */
            topic_elem = axiom_element_create(env, subs_list_node, 
                ELEM_NAME_TOPIC, ns1, &topic_node);
            topic_url_l = savan_subscriber_get_topic(subscriber, env);
            if(topic_url_l)
            {
                topic_l = savan_util_get_topic_name_from_topic_url(env, 
                    topic_url_l);
            }

            if(0 == axutil_strcmp(topic, topic_l))
            {
                AXIS2_LOG_DEBUG(env->log, AXIS2_LOG_SI, 
                "[savan]Subscribers not empty for topic :%s", topic_url);
                axiom_element_set_text(topic_elem, env, topic_url_l, topic_node); 
            }
            else
                continue;
            endto_ref = savan_subscriber_get_end_to(subscriber, env);
            endto = axis2_endpoint_ref_get_address(endto_ref, env);
            notify_ref = savan_subscriber_get_notify_to(subscriber, env);
            notify = axis2_endpoint_ref_get_address(notify_ref, env);
            filter = savan_subscriber_get_filter(subscriber, env); 
            expires = savan_subscriber_get_expires(subscriber, env);
            id = savan_subscriber_get_id(subscriber, env);
          

            /* create the subscriber element */

            sub_elem = axiom_element_create(env, subs_list_node, 
                ELEM_NAME_SUBSCRIBE, ns, &sub_node);
            
            /* Id element */
            id_elem = axiom_element_create(env, sub_node, ELEM_NAME_ID, ns1,
                &id_node);
            axiom_element_set_text(id_elem, env, id, id_node);

            /* EndTo element */
            endto_elem = axiom_element_create(env, sub_node, ELEM_NAME_ENDTO, ns,
                &endto_node);
            axiom_element_set_text(endto_elem, env, endto, endto_node);
            
            /* Delivery element */
            delivery_elem = axiom_element_create(env, sub_node, 
                ELEM_NAME_DELIVERY, ns, &delivery_node);
                
            notify_elem = axiom_element_create(env, delivery_node, 
                ELEM_NAME_NOTIFYTO, ns, &notify_node);
            axiom_element_set_text(notify_elem, env, notify, notify_node);
            
            /* Expires element */
            expires_elem = axiom_element_create(env, sub_node, 
                ELEM_NAME_EXPIRES, ns, &expires_node);
            axiom_element_set_text(expires_elem, env, expires, expires_node);
            /* Filter element */
            filter_elem = axiom_element_create(env, sub_node, ELEM_NAME_FILTER, 
                ns, &endto_node);
            axiom_element_set_text(filter_elem, env, filter, filter_node);
        }
    }
    AXIS2_LOG_DEBUG(env->log, AXIS2_LOG_SI, 
        "End:savan_subs_mgr_get_subscriber_list");
    return subs_list_node;
}

AXIS2_EXTERN axiom_node_t *AXIS2_CALL
savan_subs_mgr_get_topic_list(
    const axutil_env_t *env,
    axiom_node_t *node,
    axis2_msg_ctx_t *msg_ctx)
{

    axutil_array_list_t *topic_store = NULL;
    axiom_namespace_t *ns1 = NULL;
    axiom_node_t *topic_list_node = NULL;
    axiom_element_t* topic_list_elem = NULL;
    axis2_conf_ctx_t *conf_ctx = NULL;
    axis2_conf_t *conf = NULL;
    axis2_char_t sql_retrieve[256];
    int i = 0, size = 0;

    AXIS2_LOG_DEBUG(env->log, AXIS2_LOG_SI, 
        "Start:savan_subs_mgr_get_topic_list");

    /* Expected request format is :-
     * <ns1:get_topic_list xmlns:ns1="http://ws.apache.org/savan">
     * </ns1:get_topic_list>
     */
    conf_ctx = axis2_msg_ctx_get_conf_ctx(msg_ctx, env);
    conf = axis2_conf_ctx_get_conf(conf_ctx, env);
    if (!node) /* 'get_topic_list' node */
    {
        AXIS2_ERROR_SET(env->error, AXIS2_ERROR_SVC_SKEL_INPUT_OM_NODE_NULL, 
            AXIS2_FAILURE);
        AXIS2_LOG_ERROR(env->log, AXIS2_LOG_SI, 
            "Echo client ERROR: input parameter NULL");
        return NULL;
    }
    /* create the body of the topic_list element */
    ns1 = axiom_namespace_create (env, SAVAN_NAMESPACE, SAVAN_NS_PREFIX);
    topic_list_elem = axiom_element_create(env, NULL, ELEM_NAME_TOPICS, ns1, 
        &topic_list_node);
    sprintf(sql_retrieve, "select topic_url from topic;");

    topic_store = savan_db_mgr_retrieve_all(env, savan_util_get_dbname(env, conf),
            savan_db_mgr_topic_find_callback, sql_retrieve);

    size = axutil_array_list_size(topic_store, env);
    for(i = 0; i < size; i++)
    {
        axis2_char_t *topic_url = NULL;
        topic_url = axutil_array_list_get(topic_store, env, i); 
        AXIS2_LOG_DEBUG(env->log, AXIS2_LOG_SI, "topic_url:%s", topic_url);
        if (topic_url)
        {
            axiom_node_t *topic_node = NULL;
            axiom_element_t* topic_elem = NULL;

            /* create the topic element */
            topic_elem = axiom_element_create(env, topic_list_node, 
                ELEM_NAME_TOPIC, ns1, &topic_node);
            if(topic_node)
                axiom_element_set_text(topic_elem, env, topic_url, topic_node); 
        }
    }
    AXIS2_LOG_DEBUG(env->log, AXIS2_LOG_SI, 
        "End:savan_subs_mgr_get_topic_list");
    return topic_list_node;
}


