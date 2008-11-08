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

#include <axutil_hash.h>
#include <axis2_svc.h>
#include <axiom_element.h>
#include <axiom_node.h>
#include <axiom_soap_envelope.h>
#include <axiom_soap_body.h>
#include <axutil_uuid_gen.h>

#include <savan_sub_processor.h>
#include <savan_constants.h>
#include <savan_error.h>
#include <savan_subscriber.h>
#include <savan_util.h>
#include <savan_db_mgr.h>

struct savan_sub_processor
{
    int dummy;
};

savan_subscriber_t * AXIS2_CALL 
savan_sub_processor_create_subscriber_from_msg(
    const axutil_env_t *env,
    axis2_msg_ctx_t *msg_ctx);

axis2_status_t AXIS2_CALL
savan_sub_processor_set_sub_id_to_msg_ctx(
    const axutil_env_t *env,
    axis2_msg_ctx_t *msg_ctx,
    axis2_char_t *id);

axis2_bool_t AXIS2_CALL
savan_sub_processor_is_subscription_renewable(
    const axutil_env_t *env,
    axis2_msg_ctx_t *msg_ctx);

/* This method validates the subscription, and send a response (savan fault)
 * incase if there is a fault
 */

axis2_bool_t AXIS2_CALL
savan_sub_processor_validate_subscription(
	savan_subscriber_t *subscriber,
	const axutil_env_t *env,
	axis2_msg_ctx_t *msg_ctx);

AXIS2_EXTERN savan_sub_processor_t *AXIS2_CALL
savan_sub_processor_create(
    const axutil_env_t *env)
{
    savan_sub_processor_t *sub_processor = NULL;
    
    AXIS2_ENV_CHECK(env, NULL);
    
    sub_processor = AXIS2_MALLOC(env->allocator, 
                                 sizeof(savan_sub_processor_t));
     
    if (!sub_processor)
    { 
        AXIS2_ERROR_SET(env->error, AXIS2_ERROR_NO_MEMORY, AXIS2_FAILURE);
        return NULL;        
    }
    
    return sub_processor;
}

axis2_status_t AXIS2_CALL 
savan_sub_processor_subscribe(
    savan_sub_processor_t *sub_processor,
    const axutil_env_t *env,
    axis2_msg_ctx_t *msg_ctx)
{
    savan_subscriber_t *subscriber = NULL;
    axis2_char_t *expires = NULL;
    axis2_char_t *id = NULL;
    axis2_status_t status = AXIS2_SUCCESS;
    
    AXIS2_LOG_TRACE(env->log, AXIS2_LOG_SI, "[savan] Entry:savan_sub_processor_subscribe");
    
    /* Extract info from incoming msg and create a subscriber */
    subscriber = savan_sub_processor_create_subscriber_from_msg(env, msg_ctx);
    if (!subscriber)
    {
        AXIS2_LOG_ERROR(env->log, AXIS2_LOG_SI, "[savan] Failed to create a subscriber"); 
        AXIS2_ERROR_SET(env->error, SAVAN_ERROR_FAILED_TO_CREATE_SUBSCRIBER, AXIS2_FAILURE);
        status = axutil_error_get_status_code(env->error);
        return status;
    }    
    /* Set the expiry time on the subscription */
    /* TODO : For now we are ignoring the Expiry sent by the client. Add support
     * to consider this when setting the expiry time */

    expires = savan_util_get_expiry_time(env);
    savan_subscriber_set_expires(subscriber, env, expires);

    /*Set the filter template file for the subscriber*/

    /* Store sub id in msg ctx to be used by the msg receiver */
    id = savan_subscriber_get_id(subscriber, env);
    savan_sub_processor_set_sub_id_to_msg_ctx(env, msg_ctx, id);

	/* Validate the subscription with the available information 
 	 * If the validation fails, then, don't add the subscriber into
 	 * the list.
     */

	if (AXIS2_SUCCESS != (status = savan_sub_processor_validate_subscription(subscriber, env, 
                msg_ctx)))
	{
        savan_subscriber_free(subscriber, env);
    	AXIS2_LOG_ERROR(env->log, AXIS2_LOG_SI, 
                "[savan] Encountered a subscription validation fault.");
		return status;
	}

    if(AXIS2_SUCCESS != (status = savan_util_add_subscriber(env, msg_ctx, subscriber)))
	{
        savan_subscriber_free(subscriber, env);
		return status;
	}
    
    AXIS2_LOG_TRACE(env->log, AXIS2_LOG_SI, "[savan] Exit:savan_sub_processor_subscribe");
    return AXIS2_SUCCESS;
}

axis2_status_t AXIS2_CALL 
savan_sub_processor_unsubscribe(
    savan_sub_processor_t *sub_processor,
    const axutil_env_t *env,
    axis2_msg_ctx_t *msg_ctx)
{
    savan_subscriber_t *subscriber = NULL;
    axis2_status_t status = AXIS2_SUCCESS;
    axis2_char_t *id = NULL;

    AXIS2_LOG_TRACE(env->log, AXIS2_LOG_SI, "[savan] Entry:savan_sub_processor_unsubscribe");

    subscriber = savan_util_get_subscriber_from_msg(env, msg_ctx, NULL);
    if (!subscriber)
    {
        AXIS2_LOG_ERROR(env->log, AXIS2_LOG_SI, "[savan] Failed to find the subscriber"); 
        return AXIS2_FAILURE;
    }

    /* Store sub id in msg ctx to be used by the msg receiver */
    id = savan_subscriber_get_id(subscriber, env);
    savan_sub_processor_set_sub_id_to_msg_ctx(env, msg_ctx, id);

    /* Remove from store */
    status = savan_util_remove_subscriber(env, msg_ctx, subscriber);
    if (status != AXIS2_SUCCESS)
    {
        AXIS2_LOG_ERROR(env->log, AXIS2_LOG_SI, "[savan] Failed to remove the subscriber"); 
        return AXIS2_FAILURE;
    }

    AXIS2_LOG_TRACE(env->log, AXIS2_LOG_SI, "[savan] Exit:savan_sub_processor_unsubscribe");
    return AXIS2_SUCCESS;
}

axis2_status_t AXIS2_CALL 
savan_sub_processor_renew_subscription(
    savan_sub_processor_t *sub_processor,
    const axutil_env_t *env,
    axis2_msg_ctx_t *msg_ctx)
{
    savan_subscriber_t *subscriber = NULL;
    axis2_char_t *id = NULL;
    axis2_bool_t renewable = AXIS2_TRUE;
    axis2_char_t *expires = NULL;
    axis2_char_t *renewed_expires = NULL;
    axis2_conf_ctx_t *conf_ctx = NULL;
    axis2_conf_t *conf = NULL;
    axis2_status_t status = AXIS2_FAILURE;

    AXIS2_LOG_TRACE(env->log, AXIS2_LOG_SI, "[savan] Entry:savan_sub_processor_renew_subscription");

    subscriber = savan_util_get_subscriber_from_msg(env, msg_ctx, NULL);
    if (!subscriber)
    {
        axis2_char_t *reason = NULL;

        axutil_error_set_error_number(env->error, SAVAN_ERROR_SUBSCRIBER_NOT_FOUND); 
        reason = (axis2_char_t *) axutil_error_get_message(env->error);
        savan_util_create_fault_envelope(msg_ctx, env, SAVAN_FAULT_UTR_CODE, 
                SAVAN_FAULT_UTR_SUB_CODE, reason, SAVAN_FAULT_UTR_DETAIL1);

        AXIS2_ERROR_SET(env->error, SAVAN_ERROR_SUBSCRIBER_NOT_FOUND, AXIS2_FAILURE);
        AXIS2_LOG_ERROR(env->log, AXIS2_LOG_SI, "[savan] Failed to find the subscriber"); 
        return AXIS2_FAILURE;
    }

    /* Store sub id in msg ctx to be used by the msg receiver */
    id = savan_subscriber_get_id(subscriber, env);
    savan_sub_processor_set_sub_id_to_msg_ctx(env, msg_ctx, id);

    /* Check whether the subscription can be renewed. If renewable, set the new
     * expiry time in the subscriber */

    renewable = savan_sub_processor_is_subscription_renewable(env, msg_ctx);
    if (!renewable)
    {
        axis2_char_t *reason = NULL;

        axutil_error_set_error_number(env->error, SAVAN_ERROR_UNABLE_TO_RENEW); 
        reason = (axis2_char_t *) axutil_error_get_message(env->error);
        savan_util_create_fault_envelope(msg_ctx, env, SAVAN_FAULT_UTR_CODE,
                                         SAVAN_FAULT_UTR_SUB_CODE, 
                                         reason, 
                                         SAVAN_FAULT_UTR_DETAIL2);

        AXIS2_ERROR_SET(env->error, SAVAN_ERROR_UNABLE_TO_RENEW, AXIS2_FAILURE);
        AXIS2_LOG_ERROR(env->log, AXIS2_LOG_SI, 
                        "[savan] Subscription can not be renewed");
        savan_subscriber_set_renew_status(subscriber, env, AXIS2_FALSE);
        return AXIS2_FAILURE;
    }

    expires = savan_subscriber_get_expires(subscriber, env);
    renewed_expires = savan_util_get_renewed_expiry_time(env, expires);
    savan_subscriber_set_expires(subscriber, env, renewed_expires);
    savan_subscriber_set_renew_status(subscriber, env, AXIS2_TRUE);
    conf_ctx = axis2_msg_ctx_get_conf_ctx(msg_ctx, env);
    conf = axis2_conf_ctx_get_conf(conf_ctx, env);
    status = savan_db_mgr_update_subscriber(env, 
                                            savan_util_get_dbname(env, conf), 
                                            subscriber);

    AXIS2_LOG_TRACE(env->log, AXIS2_LOG_SI, "[savan] Exit:savan_sub_processor_renew_subscription");

    return status;
}

axis2_status_t AXIS2_CALL 
savan_sub_processor_get_status(
    savan_sub_processor_t *sub_processor,
    const axutil_env_t *env,
    axis2_msg_ctx_t *msg_ctx)
{
    axis2_char_t *id = NULL;

    AXIS2_LOG_TRACE(env->log, AXIS2_LOG_SI, "[savan] Entry:savan_sub_processor_get_status");

    /* Extract the sub id and store it in msg ctx to be used by the msg receiver */
    id = savan_util_get_subscription_id_from_msg(env, msg_ctx);
    savan_sub_processor_set_sub_id_to_msg_ctx(env, msg_ctx, id);

    AXIS2_LOG_TRACE(env->log, AXIS2_LOG_SI, "[savan] Exit:savan_sub_processor_get_status");
    return AXIS2_SUCCESS;
}

savan_subscriber_t * AXIS2_CALL 
savan_sub_processor_create_subscriber_from_msg(
    const axutil_env_t *env,
    axis2_msg_ctx_t *msg_ctx)
{
    axis2_conf_ctx_t *conf_ctx = NULL;
    axis2_conf_t *conf = NULL;
    axis2_status_t status = AXIS2_SUCCESS;
    savan_subscriber_t *subscriber = NULL;
    axis2_char_t *sub_id = NULL;
    axiom_soap_envelope_t *envelope = NULL;
    axiom_soap_body_t *body = NULL;
    axutil_qname_t *qname = NULL;
    axiom_node_t *body_node = NULL;
    axiom_node_t *sub_node = NULL;
    axiom_element_t *body_elem = NULL;
    axiom_element_t *sub_elem = NULL;
    axis2_char_t *topic_url = NULL;
    axis2_endpoint_ref_t *topic_epr = NULL;
    
    AXIS2_LOG_TRACE(env->log, AXIS2_LOG_SI, 
            "[savan] Entry:savan_sub_processor_create_subscriber_from_msg");
   
    conf_ctx = axis2_msg_ctx_get_conf_ctx(msg_ctx, env);
    conf = axis2_conf_ctx_get_conf(conf_ctx, env);
    subscriber = savan_subscriber_create(env);
    if (!subscriber)
    {
        AXIS2_LOG_ERROR(env->log, AXIS2_LOG_SI, "[savan] Failed to create a subscriber instance");
        axutil_error_set_status_code(env->error, AXIS2_FAILURE);
        return NULL;
    }
    
    /* Assign a unique id to subscriber */
    sub_id = axutil_uuid_gen(env);
    if (sub_id)
    {
        savan_subscriber_set_id(subscriber, env, sub_id);
        /* Don't free the sub_id here. we use it inside msg recv */
    }
    
    /* Get soap envelop and extract relevant elements */
   
    envelope =  axis2_msg_ctx_get_soap_envelope(msg_ctx, env);
    if (!envelope)
    {
        AXIS2_LOG_ERROR(env->log, AXIS2_LOG_SI, "[savan] Failed to extract the soap envelop"); 
        axutil_error_set_status_code(env->error, AXIS2_FAILURE);
        return NULL;
    }
    
    body = axiom_soap_envelope_get_body(envelope, env);
    if (!body)
    {
        AXIS2_LOG_ERROR(env->log, AXIS2_LOG_SI, "[savan] Failed to extract the soap body"); 
        axutil_error_set_status_code(env->error, AXIS2_FAILURE);
        return NULL;
    }
    
    /* Get Body element from body node */
    body_node = axiom_soap_body_get_base_node(body, env);
    body_elem = (axiom_element_t*)axiom_node_get_data_element(body_node, env);
    
    /* Get Subscribe element from Body */
    qname = axutil_qname_create(env, ELEM_NAME_SUBSCRIBE, EVENTING_NAMESPACE, NULL);
    sub_elem = axiom_element_get_first_child_with_qname(body_elem, env, qname,
                                                        body_node, &sub_node);
    axutil_qname_free(qname, env);
    
    /* Now read each sub element of Subscribe element */
    status = savan_util_process_subscriber_node(env, sub_node, sub_elem, subscriber);
    if(AXIS2_SUCCESS != status)
    {
        AXIS2_LOG_ERROR(env->log, AXIS2_LOG_SI, "[savan] Parsing subscriber node failed");
        axutil_error_set_status_code(env->error, AXIS2_FAILURE);
        return NULL;
    }

    topic_epr = axis2_msg_ctx_get_to(msg_ctx, env);
    if(topic_epr)
    {
        topic_url = (axis2_char_t *)axis2_endpoint_ref_get_address(topic_epr, env);
    }

    if(topic_url)
    {
        savan_subscriber_set_topic_url(subscriber, env, topic_url);
        status = savan_util_populate_topic(env, topic_url, conf);
        if(status != AXIS2_SUCCESS)
        {
            axutil_error_set_status_code(env->error, AXIS2_FAILURE);
            return NULL;
        }
    }

    AXIS2_LOG_TRACE(env->log, AXIS2_LOG_SI, 
        "[savan] Exit:savan_sub_processor_create_subscriber_from_msg");
    axutil_error_set_status_code(env->error, AXIS2_SUCCESS);
    return subscriber;    
}

axis2_status_t AXIS2_CALL
savan_sub_processor_set_sub_id_to_msg_ctx(
    const axutil_env_t *env,
    axis2_msg_ctx_t *msg_ctx,
    axis2_char_t *id)
{
    axutil_property_t *property = NULL;

    /* Set the subscription id as a property in the msg_ctx. We use this inside
     * savan_msg_recv to send the wse:Identifier
     * Pass a copy because msg ctx free function frees all properties */
    property = axutil_property_create(env);
    axutil_property_set_value(property, env, (void*)axutil_strdup(env, id));
    axis2_msg_ctx_set_property(msg_ctx, env, SAVAN_KEY_SUB_ID, property);

    return AXIS2_SUCCESS;
}

axis2_bool_t AXIS2_CALL
savan_sub_processor_is_subscription_renewable(
    const axutil_env_t *env,
    axis2_msg_ctx_t *msg_ctx)
{
    /* TODO: */

    return AXIS2_TRUE;
}

axis2_bool_t AXIS2_CALL
savan_sub_processor_validate_delivery_mode(
	savan_subscriber_t *subscriber,
	const axutil_env_t *env,
    axis2_msg_ctx_t *msg_ctx)
{
	axis2_char_t *delivery_mode = 
		savan_subscriber_get_delivery_mode(subscriber, env);

	/*if NULL we assueme, as default delivery mode*/	
	if(!delivery_mode)
	{
		return AXIS2_SUCCESS;
	}
	else if(!axutil_strcmp(delivery_mode, DEFAULT_DELIVERY_MODE))
	{
		return AXIS2_SUCCESS;	
	}
	else
	{
        axis2_char_t *reason = NULL;

        axutil_error_set_error_number(env->error, SAVAN_ERROR_REQUESTED_DELIVERY_MODE_NOT_SUPPORTED); 
        reason = (axis2_char_t *) axutil_error_get_message(env->error);
        savan_util_create_fault_envelope(msg_ctx, env,
                                         SAVAN_FAULT_DMRU_CODE, 
                                         SAVAN_FAULT_DMRU_SUB_CODE,
                                         reason, 
                                         SAVAN_FAULT_DMRU_DETAIL);

		return AXIS2_FAILURE;
	}
	return AXIS2_SUCCESS;
}

axis2_bool_t AXIS2_CALL
savan_sub_processor_validate_expiration_time(
	savan_subscriber_t *subscriber,
	const axutil_env_t *env,
    axis2_msg_ctx_t *msg_ctx)
{
	/*
      axis2_char_t *expires = savan_subscriber_get_expires(subscriber, env);	
      if(expires == NULL)
      {
      savan_util_create_fault_envelope(msg_ctx, env,
      SAVAN_FAULT_IET_CODE, SAVAN_FAULT_IET_SUB_CODE,
      SAVAN_ERROR_EXPIRATION_TIME_REQUESTED_IS_INVALID, SAVAN_FAULT_IET_DETAIL);

      return AXIS2_FAILURE;
      }
      else 
      {
      savan_util_create_fault_envelope(msg_ctx, env,
      SAVAN_FAULT_UET_CODE, SAVAN_FAULT_UET_SUB_CODE,
      SAVAN_ERROR_ONLY_EXPIRATION_DURATIONS_ARE_SUPPORTED, SAVAN_FAULT_UET_DETAIL);

      return AXIS2_FAILURE;
      }
	*/
	return AXIS2_SUCCESS;
}

axis2_bool_t AXIS2_CALL
savan_sub_processor_validate_filter(
	savan_subscriber_t *subscriber,
	const axutil_env_t *env,
    axis2_msg_ctx_t *msg_ctx)
{	
	axis2_char_t *filter = NULL;
	axis2_char_t *filter_dialect = NULL;

	filter = savan_subscriber_get_filter(subscriber, env);
	filter_dialect = savan_subscriber_get_filter_dialect(subscriber, env);
	
	if(!filter)
	{	
        AXIS2_LOG_DEBUG(env->log, AXIS2_LOG_SI, "[savan] Filter is Null");
		return AXIS2_SUCCESS;
	}
	else if(!axutil_strcmp(filter_dialect, DEFAULT_FILTER_DIALECT))
	{
        axis2_char_t *reason = NULL;

#ifdef SAVAN_FILTERING
        return AXIS2_SUCCESS;
#else
        axutil_error_set_error_number(env->error, SAVAN_ERROR_FILTERING_IS_NOT_SUPPORTED); 
        reason = (axis2_char_t *) axutil_error_get_message(env->error);
        savan_util_create_fault_envelope(msg_ctx, env,
                                         SAVAN_FAULT_FNS_CODE, 
                                         SAVAN_FAULT_FNS_SUB_CODE,
                                         reason, 
                                         SAVAN_FAULT_FNS_DETAIL);

		return AXIS2_FAILURE;
#endif
	}
	else
	{
        axis2_char_t *reason = NULL;
        
        axutil_error_set_error_number(env->error, SAVAN_ERROR_REQUESTED_FILTER_DIALECT_IS_NOT_SUPPORTED); 
        reason = (axis2_char_t *) axutil_error_get_message(env->error);
		savan_util_create_fault_envelope(msg_ctx, env,
                                         SAVAN_FAULT_FRU_CODE, 
                                         SAVAN_FAULT_FRU_SUB_CODE,
                                         reason, 
                                         SAVAN_FAULT_FRU_DETAIL);
		return AXIS2_FAILURE;
	}
}

axis2_bool_t AXIS2_CALL
savan_sub_processor_validate_subscription(
    savan_subscriber_t *subscriber,
    const axutil_env_t *env,
    axis2_msg_ctx_t *msg_ctx)
{
	if(savan_sub_processor_validate_delivery_mode(subscriber, 
                                                  env, msg_ctx) == AXIS2_FAILURE)
	{
		return AXIS2_FAILURE;
	}
		
	if(savan_sub_processor_validate_expiration_time(subscriber, 
                                                    env, msg_ctx) == AXIS2_FAILURE)
	{
		return AXIS2_FAILURE;
	}
	if(savan_sub_processor_validate_filter(subscriber, 
                                           env, msg_ctx) == AXIS2_FAILURE)
	{
		return AXIS2_FAILURE;
	}
		
	return AXIS2_SUCCESS;
}

AXIS2_EXTERN void AXIS2_CALL 
savan_sub_processor_free(
    savan_sub_processor_t * sub_processor,
    const axutil_env_t * env)
{
    if (sub_processor)
    {
        AXIS2_FREE(env->allocator, sub_processor);
    }
}

