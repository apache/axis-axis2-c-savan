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

#include <axis2_handler_desc.h>
#include <axutil_array_list.h>
#include <axiom_soap_const.h>
#include <axiom_soap_envelope.h>
#include <axiom_soap_header.h>
#include <axiom_soap_header_block.h>
#include <axis2_op.h>
#include <axis2_msg_ctx.h>
#include <axis2_conf_ctx.h>
#include <axis2_msg_info_headers.h>
#include <axutil_property.h>
#include <axis2_engine.h>
#include <axis2_svc.h>

#include <savan_constants.h>
#include <savan_error.h>
#include <savan_sub_processor.h>
#include <savan_util.h>
#include <savan_msg_recv.h>
#include <savan_storage_mgr.h>

axis2_status_t AXIS2_CALL
savan_in_handler_invoke(struct axis2_handler *handler, 
                        const axutil_env_t *env,
                        struct axis2_msg_ctx *msg_ctx);


AXIS2_EXTERN axis2_handler_t* AXIS2_CALL
savan_in_handler_create(const axutil_env_t *env, 
                        axutil_qname_t *qname) 
{
    axis2_handler_t *handler = NULL;
    
    handler = axis2_handler_create(env);
    if (!handler)
    {
        return NULL;
    }
   
    /* Handler init is handled by conf loading, so no need to do it here */
    
    /* Set the base struct's invoke op */
	axis2_handler_set_invoke(handler, env, savan_in_handler_invoke);

    return handler;
}

axis2_status_t AXIS2_CALL
savan_in_handler_invoke(struct axis2_handler *handler, 
                        const axutil_env_t *env,
                        struct axis2_msg_ctx *msg_ctx)
{
    savan_message_types_t msg_type = SAVAN_MSG_TYPE_UNKNOWN;
    savan_sub_processor_t *processor = NULL;
    axis2_status_t status = AXIS2_SUCCESS;
    axis2_bool_t to_msg_recv = AXIS2_FALSE;
    axis2_conf_ctx_t *conf_ctx = NULL;
    axis2_conf_t *conf = NULL;
    int type;
    savan_storage_mgr_t *storage_mgr = NULL;
    
    AXIS2_LOG_TRACE(env->log, AXIS2_LOG_SI, "[savan] Start:savan_in_handler_invoke");

    AXIS2_PARAM_CHECK(env->error, msg_ctx, AXIS2_FAILURE);

    conf_ctx = axis2_msg_ctx_get_conf_ctx(msg_ctx, env);
    conf = axis2_conf_ctx_get_conf(conf_ctx, env);

    storage_mgr = savan_util_get_storage_mgr(env, conf_ctx, conf);
    if(!storage_mgr)
    {
        AXIS2_LOG_ERROR(env->log, AXIS2_LOG_SI, "[savan] Could not create the database. Check \
            whether database path is correct and accessible. Exit loading the Savan module");
        AXIS2_LOG_HANDLE(env, SAVAN_ERROR_DATABASE_CREATION_ERROR, AXIS2_FAILURE);

        return AXIS2_FAILURE;
    }
    
    /* create a subscription processor */
    processor = savan_sub_processor_create(env, storage_mgr);
    if (!processor)
    {
        AXIS2_LOG_ERROR(env->log, AXIS2_LOG_SI, "[savan] Failed to create subscription processor");
        return AXIS2_SUCCESS; /* returning FAILURE will break handler chain */
    }
    
    /* determine the eventing msg type */
    msg_type = savan_util_get_message_type(msg_ctx, env);
    type = (int)msg_type;

    switch (type)
    {
        case SAVAN_MSG_TYPE_SUB:
        {
            if(savan_sub_processor_subscribe(processor, 
                env, msg_ctx) == AXIS2_FAILURE)
            {
                status = AXIS2_FAILURE;
            }
            else
            {
                to_msg_recv = AXIS2_TRUE;
            }
            break;
        }

        case SAVAN_MSG_TYPE_UNSUB:
        {
            status = savan_sub_processor_unsubscribe(processor, env, msg_ctx);
            to_msg_recv = AXIS2_TRUE;
            break;
        }

        case SAVAN_MSG_TYPE_RENEW:
        {
            if(savan_sub_processor_renew_subscription(processor, env, msg_ctx) == AXIS2_FAILURE)
            {
                to_msg_recv = AXIS2_FALSE;
                status = AXIS2_FAILURE;
            }
            else
            {
                to_msg_recv = AXIS2_TRUE;
            }
            break;
        }

        case SAVAN_MSG_TYPE_GET_STATUS:
        {
            status = savan_sub_processor_get_status(processor, env, msg_ctx);
            to_msg_recv = AXIS2_TRUE;
            break;
        }

        case SAVAN_MSG_TYPE_UNKNOWN:
        {
            /* not an error. just log it */
            AXIS2_LOG_INFO(env->log, "[savan][in handler] Unhandled message type"); 
            status = AXIS2_SUCCESS;
            break;
        }
    }    
    
    if (to_msg_recv)
    {
        axis2_op_t *op =  axis2_msg_ctx_get_op(msg_ctx, env);
        axis2_msg_recv_t* msg_recv = savan_msg_recv_create(env);
        axis2_op_set_msg_recv(op, env, msg_recv);
    }
    
    savan_sub_processor_free(processor, env);
    AXIS2_LOG_TRACE(env->log, AXIS2_LOG_SI, "[savan] End:savan_in_handler_invoke");
    
    return status;
}

