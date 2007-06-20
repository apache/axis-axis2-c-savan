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

/*
#include <axis2_addr_mod.h>
#include <axis2_addr.h>
*/
#include <axis2_handler_desc.h>
#include <axutil_array_list.h>
#include <axiom_soap_const.h>
#include <axiom_soap_envelope.h>
#include <axiom_soap_header.h>
#include <axiom_soap_body.h>
#include <axiom_soap_header_block.h>
#include <axis2_op.h>
#include <axis2_msg_ctx.h>
#include <axis2_conf_ctx.h>
#include <axis2_msg_info_headers.h>
#include <axutil_property.h>

#include <savan_constants.h>
#include <savan_error.h>
#include <savan_sub_processor.h>
#include <savan_util.h>
#include <savan_msg_recv.h>
#include <savan_subscriber.h>

/* Function Prototypes ********************************************************/

axis2_status_t AXIS2_CALL
savan_out_handler_invoke(
    struct axis2_handler *handler, 
    const axutil_env_t *env,
    struct axis2_msg_ctx *msg_ctx);

/* End of Function Prototypes *************************************************/
                         
AXIS2_EXTERN axis2_handler_t* AXIS2_CALL
savan_out_handler_create(
    const axutil_env_t *env, 
    axutil_qname_t *qname) 
{
    axis2_handler_t *handler = NULL;
    
    AXIS2_ENV_CHECK(env, NULL);
    
    handler = axis2_handler_create(env);
    if (!handler)
    {
        return NULL;
    }
   
    /* handler init is handled by conf loading, so no need to do it here */
    
    /* set the base struct's invoke op */
	axis2_handler_set_invoke(handler, env,savan_out_handler_invoke);

    return handler;
}

/******************************************************************************/

axis2_status_t AXIS2_CALL
savan_out_handler_invoke(
    struct axis2_handler *handler, 
    const axutil_env_t *env,
    struct axis2_msg_ctx *msg_ctx)
{
    savan_message_types_t msg_type = SAVAN_MSG_TYPE_UNKNOWN;
    
    AXIS2_ENV_CHECK( env, AXIS2_FAILURE);
    AXIS2_PARAM_CHECK(env->error, msg_ctx, AXIS2_FAILURE);
    
    /* Determine the eventing msg type */
    msg_type = savan_util_get_message_type(msg_ctx, env);
    AXIS2_LOG_DEBUG(env->log, AXIS2_LOG_SI, "[savan][out handler] msg type:"
        " %d", msg_type);
    if (msg_type == SAVAN_MSG_TYPE_UNKNOWN)
    {
        axutil_property_t *subs_list_property = NULL;
        axutil_hash_t *subscriber_list = NULL;
        axutil_hash_index_t *hi = NULL;
        void *val = NULL;
        /* Treat unknown msgs as msgs for publishing */
        subs_list_property = axis2_msg_ctx_get_property(msg_ctx, env, 
            SAVAN_SUBSCRIBER_LIST);
        if(subs_list_property)
            subscriber_list = axutil_property_get_value(subs_list_property, env);    
        if (!subscriber_list)
        {
            AXIS2_LOG_ERROR(env->log, AXIS2_LOG_SI, "[savan][out handler] "
                "Subscribe subscriber_list is null");
            return AXIS2_SUCCESS; /* returning FAILURE will break handler chain */
        }
        
        /* Iterate the subscribe store and send the msg to each one */

        for (hi = axutil_hash_first(subscriber_list, env); hi; hi = 
            axutil_hash_next(env, hi))
        {
            axiom_soap_envelope_t *soap_env = NULL;
            axiom_soap_body_t *soap_body = NULL;
            axiom_node_t *payload = NULL;
            savan_subscriber_t * sub = NULL;
            axutil_hash_this(hi, NULL, NULL, &val);
            sub = (savan_subscriber_t *)val;
            if (sub)
            {
                axis2_char_t *id = savan_subscriber_get_id(sub, env);
                AXIS2_LOG_DEBUG(env->log, AXIS2_LOG_SI, "[savan][out handler] "
                    "Publishing to %s...", id);
                soap_env = axis2_msg_ctx_get_soap_envelope(msg_ctx, env);
                soap_body = axiom_soap_envelope_get_body(soap_env, env);
                payload = axiom_soap_body_get_base_node(soap_body, env);
                savan_subscriber_publish(sub, env, payload);
            }

            val = NULL;
        }


        axis2_msg_ctx_set_paused(msg_ctx, env, AXIS2_TRUE);
    }
       
    return AXIS2_SUCCESS;
}

