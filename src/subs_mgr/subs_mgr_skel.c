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
#include <stdio.h>
#include <axis2_svc_skeleton.h>
#include <axutil_array_list.h>
#include <axiom_element.h>
#include <axiom_soap_envelope.h>
#include <axiom_soap_header.h>
#include <axiom_soap_body.h>
#include <axiom_soap_fault.h>
#include <axiom_stax_builder.h>
#include <axis2_engine.h>
#include <axis2_conf_ctx.h>
#include <axis2_conf_init.h>
#include <axutil_thread_pool.h>
#include <axis2_util.h>
#include <axis2_svc_client.h>
#include <axis2_options.h>
#include <savan_constants.h>

#include "savan_subs_mgr.h"

int AXIS2_CALL
savan_subs_mgr_free(
    axis2_svc_skeleton_t *svc_skeleton,
    const axutil_env_t *env);

axis2_status_t AXIS2_CALL
savan_subs_mgr_free_void_arg(
    void *svc_skeleton,
    const axutil_env_t *env);

/*
 * This method invokes the right service method 
 */
axiom_node_t* AXIS2_CALL 
savan_subs_mgr_invoke(
    axis2_svc_skeleton_t *svc_skeleton,
    const axutil_env_t *env,
    axiom_node_t *node,
    axis2_msg_ctx_t *msg_ctx);
            

int AXIS2_CALL 
savan_subs_mgr_init(
    axis2_svc_skeleton_t *svc_skeleton,
    const axutil_env_t *env);

int AXIS2_CALL 
savan_subs_mgr_init_with_conf(
    axis2_svc_skeleton_t *svc_skeleton,
    const axutil_env_t *env,
    axis2_conf_t *conf);

axiom_node_t* AXIS2_CALL
savan_subs_mgr_on_fault(
    axis2_svc_skeleton_t *svc_skeli, 
    const axutil_env_t *env, 
    axiom_node_t *node);

static const axis2_svc_skeleton_ops_t savan_subs_mgr_skeleton_ops_var = {
    savan_subs_mgr_init,
    savan_subs_mgr_invoke,
    savan_subs_mgr_on_fault,
    savan_subs_mgr_free,
    savan_subs_mgr_init_with_conf
};

/*Create function */
axis2_svc_skeleton_t *
savan_subs_mgr_create(
    const axutil_env_t *env)
{

    axis2_svc_skeleton_t *svc_skeleton = NULL;

	
	AXIS2_LOG_DEBUG(env->log, AXIS2_LOG_SI, "savan_subs_mgr service create called");

	/* Allocate memory for the structs */
    svc_skeleton = AXIS2_MALLOC(env->allocator, sizeof(axis2_svc_skeleton_t));

    svc_skeleton->ops = &savan_subs_mgr_skeleton_ops_var;
    svc_skeleton->func_array = NULL;

    /* Assign function pointers */
    
    return svc_skeleton;
}

/* Initialize the service */
int AXIS2_CALL
savan_subs_mgr_init(
    axis2_svc_skeleton_t *svc_skeleton,
    const axutil_env_t *env)
{
    svc_skeleton->func_array = axutil_array_list_create(env, 0);
    /* Add the implemented operation names of the service to
     * the array list of functions
     */
    axutil_array_list_add(svc_skeleton->func_array, env, "add_subscriber");
    axutil_array_list_add(svc_skeleton->func_array, env, "remove_subscriber");
    axutil_array_list_add(svc_skeleton->func_array, env, "get_subscriber_list");
    axutil_array_list_add(svc_skeleton->func_array, env, "add_topic");
    axutil_array_list_add(svc_skeleton->func_array, env, "get_topic_list");

    return AXIS2_SUCCESS;
}

int AXIS2_CALL 
savan_subs_mgr_init_with_conf(
    axis2_svc_skeleton_t *svc_skeleton,
    const axutil_env_t *env,
    axis2_conf_t *conf)
{
    axutil_array_list_t *topic_param_list = NULL;
    axutil_hash_t *topic_list = NULL;
    axis2_svc_t *subs_svc = NULL;
    axis2_op_t *op = NULL;
    axutil_param_t *param = NULL;
    int i = 0, size = 0;

    savan_subs_mgr_init(svc_skeleton, env);
    subs_svc = axis2_conf_get_svc(conf, env, "subscription");
    param = axis2_svc_get_param(subs_svc, env, SAVAN_TOPIC_LIST);
    if (!param)
    {
        /* Store not found. Create and set it as a param */
        savan_util_set_store(subs_svc, env, SAVAN_TOPIC_LIST);
        param = axis2_svc_get_param(subs_svc, env, SAVAN_TOPIC_LIST);
    }
    topic_list = (axutil_hash_t*)axutil_param_get_value(param, env);
    if(!topic_list)
    {
        AXIS2_LOG_ERROR(env->log, AXIS2_LOG_SI, 
            "[savan] Failed to extract the topic list"); 
        return AXIS2_FAILURE;
    }
    op = axis2_svc_get_op_with_name(subs_svc, env, "get_topic_list");
    topic_param_list = axis2_op_get_all_params(op, env);
    if(topic_param_list)
        size = axutil_array_list_size(topic_param_list, env);
    for(i = 0; i < size; i++)
    {
        axutil_param_t *topic_param = NULL;
        axis2_char_t *topic_url_str = NULL;
        axis2_char_t *topic_name = NULL;
        axutil_hash_t *subs_list = NULL;
        topic_param = axutil_array_list_get(topic_param_list, env, i);
        topic_url_str = axutil_param_get_value(topic_param, env);
        topic_name = axutil_param_get_name(topic_param, env);
        subs_list = axutil_hash_get(topic_list, topic_name, 
            AXIS2_HASH_KEY_STRING);
        if(!subs_list)
        {
            subs_list = axutil_hash_make(env);
            axutil_hash_set(topic_list, topic_name, AXIS2_HASH_KEY_STRING, 
                subs_list);
            AXIS2_LOG_DEBUG(env->log, AXIS2_LOG_SI, 
                "[savan] Topic:%s is added to the topic list", topic_name);
        }

    }
    return AXIS2_SUCCESS;
}

/*
 * This method invokes the right service method 
 */
axiom_node_t* AXIS2_CALL
savan_subs_mgr_invoke(
    axis2_svc_skeleton_t *svc_skeleton,
    const axutil_env_t *env,
    axiom_node_t *node,
    axis2_msg_ctx_t *msg_ctx)
{
    /* Invoke the business logic.
     * Depending on the function name invoke the correct impl method.
     */
    axis2_op_t *op = NULL;
    axutil_qname_t *op_qname = NULL;
    axis2_char_t *op_name = NULL;
    
    op = axis2_msg_ctx_get_op(msg_ctx, env);
    if(op)
    {
        op_qname = (axutil_qname_t *)axis2_op_get_qname(op, env);
        if(op_qname)
            op_name = axutil_qname_get_localpart(op_qname, env);
        if(op_name)
        {
            if (axutil_strcmp(op_name, "add_subscriber") == 0)
                return savan_subs_mgr_add_subscriber(env, node, msg_ctx);
            if (axutil_strcmp(op_name, "remove_subscriber") == 0)
                return savan_subs_mgr_remove_subscriber(env, node, msg_ctx);
            if (axutil_strcmp(op_name, "get_subscriber_list") == 0)
                return savan_subs_mgr_get_subscriber_list(env, node, msg_ctx);
            /*if (axutil_strcmp(op_name, "add_topic") == 0)
                return savan_subs_mgr_add_topic(env, node, msg_ctx);
            if (axutil_strcmp(op_name, "get_topic_list") == 0)
                return savan_subs_mgr_get_topic_list(env, node, msg_ctx);*/
        }
    }
    return NULL;
}

/* On fault, handle the fault */
axiom_node_t* AXIS2_CALL
savan_subs_mgr_on_fault(
    axis2_svc_skeleton_t *svc_skeli, 
    const axutil_env_t *env, 
    axiom_node_t *node)
{
   /* Here we are just setting a simple error message inside an element 
    * called 'EchoServiceError' 
    */
    axiom_node_t *error_node = NULL;
    axiom_node_t* text_node = NULL;
    axiom_element_t *error_ele = NULL;
    error_ele = axiom_element_create(env, node, "TestServiceError", NULL, 
        &error_node);
    axiom_element_set_text(error_ele, env, "Test service failed ", 
        text_node);
    return error_node;
}

/* Free the resources used */
int AXIS2_CALL
savan_subs_mgr_free(
    axis2_svc_skeleton_t *svc_skeleton,
    const axutil_env_t *env)
{
    /* Free the function array */
    if(svc_skeleton->func_array)
    {
        axutil_array_list_free(svc_skeleton->func_array, env);
    }
    
    /* Free the service skeleton */
    if(svc_skeleton)
    {
        AXIS2_FREE(env->allocator, svc_skeleton);
    }

    return AXIS2_SUCCESS; 
}

/**
 * Following block distinguish the exposed part of the dll.
 */
AXIS2_EXPORT int 
axis2_get_instance(
    axis2_svc_skeleton_t **inst,
    const axutil_env_t *env)
{
   *inst = savan_subs_mgr_create(env);
    if(!(*inst))
    {
        return AXIS2_FAILURE;
    }

    return AXIS2_SUCCESS;
}

AXIS2_EXPORT int 
axis2_remove_instance(
    axis2_svc_skeleton_t *inst,
    const axutil_env_t *env)
{
    axis2_status_t status = AXIS2_FAILURE;
    if (inst)
    {
        status = AXIS2_SVC_SKELETON_FREE(inst, env);
    }
    return status;
}

