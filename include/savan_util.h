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
 
#ifndef SAVAN_UTIL_H
#define SAVAN_UTIL_H

#include <axis2_const.h>
#include <axutil_error.h>
#include <axutil_hash.h>
#include <axis2_defines.h>
#include <axutil_utils_defines.h>
#include <axutil_env.h>
#include <axutil_allocator.h>
#include <axis2_msg_ctx.h>

#include <savan_constants.h>
#include <savan_subscriber.h>
#include <savan_sub_processor.h>
#include <axiom_node.h>
#include <axiom_element.h>

#ifdef __cplusplus
extern "C"
{
#endif

/**
 * @defgroup savan_util
 * @ingroup Savan Util
 * @{
 */

	/**
     * Create the fault envelope, to be sent
     * to the client.
     * @param msg_ctx msg context
     * @param env environment
     * @param code, fault code
     * @param subcode, fault sub code
     * @param reason, fault reason
     * @param detail, fault deails.
	*/

	axis2_status_t AXIS2_CALL
	savan_util_create_fault_envelope(
		axis2_msg_ctx_t *msg_ctx,
		const axutil_env_t *env,
		axis2_char_t *code,
		axis2_char_t *subcode,
		axis2_char_t *reason,
		axis2_char_t *detail);

	/**
 	* Build a savan fault message and send.
 	* @param env, pointer to the environment
 	* @param code, SOAP12:Sender
 	* @param subcode, fault subcode
 	* @param reason, fault reason
 	* @param detail, details about fault,
 	* and solution to avoid.
 	*/ 

    /*
	int AXIS2_CALL
	savan_util_send_fault_notification(
    	savan_subscriber_t *subscriber,
    	const axutil_env_t *env,
    	axis2_char_t * code,
    	axis2_char_t * subcode,
    	axis2_char_t * reason,
    	axis2_char_t * detail);
    */

	/**
 	* Build a savan fault message
 	* @param env, pointer to the environment
 	* @param code, SOAP12:Sender
 	* @param subcode, fault subcode
 	* @param reason, fault reason
 	* @param detail, details about fault,
 	* and solution to avoid.
 	*/ 

	axiom_node_t * AXIS2_CALL
	savan_util_build_fault_msg(
		const axutil_env_t *env,
		axis2_char_t * code,
		axis2_char_t * subcode,
		axis2_char_t * reason,
		axis2_char_t * detail);

	/**
 	* Apply the filter against the subscriber
 	* @param subscriber, pointer to the subscriber
 	* @param env, pointer to the environment
 	* @param payload, pointer to the payload.
 	* returns filtered payload. 
 	* If filtered payload is not NULL send it to the sink.
 	* Else nothing is sent.
 	*/ 

	axiom_node_t *AXIS2_CALL
	savan_util_apply_filter(
    	savan_subscriber_t *subscriber,
    	const axutil_env_t *env,
    	axiom_node_t *payload);

	/**
 	* Set the filter template for the subscriber
 	* for filtering.
 	* @param subscriber pointer to the subscriber
 	* @param sub_processor, pointer to the sub_processor
 	* @param env, pointer to the environment.
 	* returns success, if operation is successful.
 	*/

    #ifdef SAVAN_FILTERING
    axis2_status_t AXIS2_CALL
    savan_util_set_filter_template_for_subscriber(
        savan_subscriber_t *subscriber,
        const axutil_env_t *env);
    #endif

    savan_message_types_t AXIS2_CALL
    savan_util_get_message_type(
        axis2_msg_ctx_t *msg_ctx,
        const axutil_env_t *env);
    
    /**
    * Extracts the subscription ID from the given messsage
    * context.
    * @param env pointer to environment struct
    * @param msg_ctx pointer to message context
    * @return the ID on success, else NULL
    */

    axis2_char_t * AXIS2_CALL
    savan_util_get_subscription_id_from_msg(
        const axutil_env_t *env,
        axis2_msg_ctx_t *msg_ctx);

    /**
    * Find the subscriber object from the store using the given messsage
    * context.
    * @param env pointer to environment struct
    * @param msg_ctx pointer to message context
    * @param sub_id pointer to subscription id 
    * @return a pointer to subscriber on success, else NULL
    */

    savan_subscriber_t * AXIS2_CALL
    savan_util_get_subscriber_from_msg(
        const axutil_env_t *env,
        axis2_msg_ctx_t *msg_ctx,
        axis2_char_t *sub_id);

    /**
    * Get the subscriber store from the service
    * Note that if the subscription manager is a separate service from
    * the publisher service then both SubscriptionMgrName and SubscriptionMgrURL
    * must be set in the publishers services.xml
    * @param env pointer to environment struct
    * @param msg_ctx pointer to message context
    * @return the store on success, else NULL
    */

    axutil_hash_t * AXIS2_CALL
    savan_util_get_subscriber_store(
        const axutil_env_t *env,
        axis2_msg_ctx_t *msg_ctx);

    /**
    * Add the subscriber to subscription manager services' store
    * Note that if the subscription manager is a separate service from
    * the publisher service then both SubscriptionMgrName and SubscriptionMgrURL
    * must be set in the publishers services.xml
    * @param env pointer to environment struct
    * @param msg_ctx pointer to message context
    * @param subscriber
    * @return the store on success, else NULL
    */

    axis2_status_t AXIS2_CALL
    savan_util_add_subscriber(
        const axutil_env_t *env,
        axis2_msg_ctx_t *msg_ctx,
        savan_subscriber_t *subscriber);

    axis2_status_t AXIS2_CALL
    savan_util_update_subscriber(
        const axutil_env_t *env,
        axis2_msg_ctx_t *msg_ctx,
        savan_subscriber_t *subscriber);

    /**
    * Remove the subscriber from subscription manager services' store
    * Note that if the subscription manager is a separate service from
    * the publisher service then both SubscriptionMgrName and SubscriptionMgrURL
    * must be set in the publishers services.xml
    * @param env pointer to environment struct
    * @param msg_ctx pointer to message context
    * @param subscriber
    * @return the store on success, else NULL
    */

    axis2_status_t AXIS2_CALL
    savan_util_remove_subscriber(
        const axutil_env_t *env,
        axis2_msg_ctx_t *msg_ctx,
        savan_subscriber_t *subscriber);

    /**
    * Calculate and return an expiry time for the subscription
    * @param env pointer to environment struct
    * @return the expiry time on success, else NULL
    */

    axis2_char_t * AXIS2_CALL
    savan_util_get_expiry_time(
        const axutil_env_t *env);

     /**
    * Calculate and return a new expiry time for the subscription based on the
    * current expiry time.
    * @param env pointer to environment struct
    * @param expiry current expiry time
    * @return the new expiry time on success, else NULL
    */
    axis2_char_t * AXIS2_CALL
    savan_util_get_renewed_expiry_time(
        const axutil_env_t *env,
        axis2_char_t *expiry);
    
    /**
    * Create storage hash and set as a service parameter.
    * @param env pointer to environment struct
    * @param svc subscription service
    * @return AXIS2_SUCCESS on success, else AXIS2_FAILURE 
    */
    AXIS2_EXTERN axis2_status_t AXIS2_CALL 
    savan_util_set_store(
        axis2_svc_t *svc,
        const axutil_env_t *env,
        axis2_char_t *store_name);

    /**
    * Get the subscribers registered for a topic
    * @param env pointer to environment struct
    * @param topic topic for which the subscribers are registered
    * @param subs_mgr_url url of the subscription manager
    * @return subscribers in a array list
    */
    AXIS2_EXTERN axutil_array_list_t *AXIS2_CALL
    savan_util_get_subscriber_list_from_remote_subs_mgr(
        const axutil_env_t *env,
        axis2_char_t *topic,
        axis2_char_t *subs_mgr_url,
        void *svc_client,
        axis2_conf_t *conf);

    AXIS2_EXTERN axis2_char_t *AXIS2_CALL
    savan_util_get_topic_name_from_topic_url(
        const axutil_env_t *env,
        axis2_char_t *topic_url);

    AXIS2_EXTERN axis2_char_t *AXIS2_CALL
    savan_util_get_dbname(
        const axutil_env_t *env,
        axis2_conf_t *conf);

    /**
    * Get the module parameter value by passing the module parameter name.
    * @param env pointer to environment struct
    * @param conf Axis2/C configuration structure
    * @param name module parameter name
    * @return module parameter value
    */
    AXIS2_EXTERN axis2_char_t *AXIS2_CALL
    savan_util_get_module_param(
        const axutil_env_t *env,
        axis2_conf_t *conf,
        axis2_char_t *name);

    /**
    * Get the topics registered in a subscription manager
    * @param env pointer to environment struct
    * @param subs_mgr_url url of the subscription manager
    * @return subscribers in a array list
    */
    axutil_array_list_t *AXIS2_CALL
    savan_util_get_topic_list_from_remote_subs_mgr(
        const axutil_env_t *env,
        axis2_char_t *subs_mgr_url,
        void *s_client);

    savan_subscriber_t *AXIS2_CALL
    savan_util_get_subscriber_from_remote_subs_mgr(
        const axutil_env_t *env,
        axis2_char_t *subs_id,
        axis2_char_t *subs_mgr_url,
        void *s_client,
        axis2_conf_t *conf);

    void *AXIS2_CALL
    savan_util_get_svc_client(
        const axutil_env_t *env);

    AXIS2_EXTERN axis2_status_t AXIS2_CALL
    savan_util_process_subscriber_node(
        const axutil_env_t *env,
        axiom_node_t *sub_node,
        axiom_element_t *sub_elem,
        savan_subscriber_t *subscriber);

    AXIS2_EXTERN savan_subscriber_t * AXIS2_CALL
    savan_util_process_savan_specific_subscriber_node(
        const axutil_env_t *env,
        axiom_node_t *sub_node,
        axis2_conf_t *conf);

    AXIS2_EXTERN axiom_node_t * AXIS2_CALL
    savan_util_create_subscriber_node(
        const axutil_env_t *env,
        savan_subscriber_t *subscriber,
        axiom_node_t *parent_node);

    AXIS2_EXTERN axiom_node_t * AXIS2_CALL
    savan_util_create_savan_specific_subscriber_node(
        const axutil_env_t *env, 
        savan_subscriber_t *subscriber,
        axiom_node_t *parent_node);

    AXIS2_EXTERN axis2_status_t AXIS2_CALL
    savan_util_populate_topic(
        const axutil_env_t *env,
        axis2_char_t *topic_url,
        axis2_conf_t *conf);

/** @} */
#ifdef __cplusplus
}
#endif
 
#endif /*SAVAN_UTIL_H*/
