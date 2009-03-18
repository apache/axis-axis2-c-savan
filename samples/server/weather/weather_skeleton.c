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
#include <axis2_util.h>
#include <axis2_svc_skeleton.h>
#include <axiom_element.h>
#include <savan_publishing_client.h>

#include "weather.h"

#define WEATHER_STATUS "weather_status"
#define WEATHER "weather"

typedef struct weather_data
{
    axutil_env_t *env;
    axis2_conf_t *conf;
}weather_data_t;

int AXIS2_CALL
weather_free(axis2_svc_skeleton_t *svc_skeleton,
            const axutil_env_t *env);

axis2_status_t AXIS2_CALL
weather_free_void_arg(void *svc_skeleton,
                    const axutil_env_t *env);

/*
 * This method invokes the right service method 
 */
axiom_node_t* AXIS2_CALL 
weather_invoke(
    axis2_svc_skeleton_t *svc_skeleton,
    const axutil_env_t *env,
    axiom_node_t *node,
    axis2_msg_ctx_t *msg_ctx);
        

int AXIS2_CALL 
weather_init(axis2_svc_skeleton_t *svc_skeleton,
          const axutil_env_t *env);

static void
send_weather_event(
    const axutil_env_t *env,
    axis2_conf_t *conf);

int AXIS2_CALL 
weather_init_with_conf(
    axis2_svc_skeleton_t *svc_skeleton,
    const axutil_env_t *env,
    axis2_conf_t *conf);

axiom_node_t* AXIS2_CALL
weather_on_fault(axis2_svc_skeleton_t *svc_skeli, 
    const axutil_env_t *env, axiom_node_t *node);

static const axis2_svc_skeleton_ops_t weather_skeleton_ops_var = {
    weather_init,
    weather_invoke,
    weather_on_fault,
    weather_free,
    weather_init_with_conf
};
    
/*Create function */
axis2_svc_skeleton_t *
axis2_weather_create(const axutil_env_t *env)
{

	axis2_svc_skeleton_t *svc_skeleton = NULL;

    /* Allocate memory for the structs */
    svc_skeleton = AXIS2_MALLOC(env->allocator, 
        sizeof(axis2_svc_skeleton_t));

    svc_skeleton->ops = &weather_skeleton_ops_var;
    svc_skeleton->func_array = NULL;

    /* Assign function pointers */
    

    return svc_skeleton;
}

/* Initialize the service */
int AXIS2_CALL
weather_init(axis2_svc_skeleton_t *svc_skeleton,
                        const axutil_env_t *env)
{
    svc_skeleton->func_array = axutil_array_list_create(env, 0);

    /* Add the implemented operation names of the service to  
     * the array list of functions 
     */

    axutil_array_list_add(svc_skeleton->func_array, env, "send");

    /* Any initialization stuff of service should go here */

    return AXIS2_SUCCESS;
}

int AXIS2_CALL 
weather_init_with_conf(
    axis2_svc_skeleton_t *svc_skeleton,
    const axutil_env_t *env,
    axis2_conf_t *conf)
{
    weather_init(svc_skeleton, env);
    /*send_weather_event(env, conf); */
    return AXIS2_SUCCESS;
}

/*
 * This method invokes the right service method 
 */
axiom_node_t* AXIS2_CALL
weather_invoke(
    axis2_svc_skeleton_t *svc_skeleton,
    const axutil_env_t *env,
    axiom_node_t *node,
    axis2_msg_ctx_t *msg_ctx)
{
    axis2_conf_t *conf = NULL;
    axis2_conf_ctx_t *conf_ctx = NULL;

    conf_ctx = axis2_msg_ctx_get_conf_ctx(msg_ctx, env);
    conf = axis2_conf_ctx_get_conf(conf_ctx, env);
    /* Depending on the function name invoke the
     *  corresponding function
     */
    if (node)
    {
        AXIS2_LOG_DEBUG(env->log, AXIS2_LOG_SI, "node:%s", axiom_node_to_string(node, env));
        if (axiom_node_get_node_type(node, env) == AXIOM_ELEMENT)
        {
            axiom_element_t *element = NULL;
            element =
                (axiom_element_t *) axiom_node_get_data_element(node, env);
            if (element)
            {
                axis2_char_t *op_name =
                    axiom_element_get_localname(element, env);
                AXIS2_LOG_DEBUG(env->log, AXIS2_LOG_SI, "op_name:%s", op_name);
                if (op_name)
                {
                    if (axutil_strcmp(op_name, "send") == 0)
                    {
                        send_weather_event(env, conf); 
                        return axis2_weather_send(env, node);
                    }
                }
            }
        }
    }

    AXIS2_LOG_ERROR(env->log, AXIS2_LOG_SI, 
            "[savan] Weather service ERROR: invalid OM parameters in request");

    return NULL;
}

/* On fault, handle the fault */
axiom_node_t* AXIS2_CALL
weather_on_fault(axis2_svc_skeleton_t *svc_skeli, 
              const axutil_env_t *env, axiom_node_t *node)
{
   /* Here we are just setting a simple error message inside an element 
    * called 'EchoServiceError' 
    */
    axiom_node_t *error_node = NULL;
    axiom_node_t* text_node = NULL;
    axiom_element_t *error_ele = NULL;
    error_ele = axiom_element_create(env, node, "WeatherServiceError", NULL, 
        &error_node);
    axiom_element_set_text(error_ele, env, "Weather service failed ", 
        text_node);
    return error_node;
}

/* Free the resources used */
int AXIS2_CALL
weather_free(axis2_svc_skeleton_t *svc_skeleton,
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

static void
send_weather_event(
    const axutil_env_t *env,
    axis2_conf_t *conf)
{
    axiom_namespace_t *test_ns = NULL;
    axiom_element_t* test_elem = NULL;
    axiom_node_t *test_node = NULL;
    axiom_element_t* test_elem1 = NULL;
    axiom_node_t *test_node1 = NULL;
    axis2_svc_t *svc = NULL;
    savan_publishing_client_t *pub_client = NULL;
    
    svc = axis2_conf_get_svc(conf, env, WEATHER);

    pub_client = savan_publishing_client_create(env, conf, svc);
    /* Build a payload and pass it to the savan publishing client */ 
    test_ns = axiom_namespace_create (env, "http://www.wso2.com/savan/c/weather", "weather");
    test_elem = axiom_element_create(env, NULL, "weather", test_ns, &test_node);
    test_elem1 = axiom_element_create(env, test_node, "weather_report", NULL, &test_node1);

    axiom_element_set_text(test_elem1, env, "sunny day", test_node1);

    savan_publishing_client_publish(pub_client, env, test_node, NULL);
    savan_publishing_client_free(pub_client, env);
    
    return;
}

/**
 * Following block distinguish the exposed part of the dll.
 */
AXIS2_EXPORT int 
axis2_get_instance(axis2_svc_skeleton_t **inst,
                   const axutil_env_t *env)
{
   *inst = axis2_weather_create(env);
    if(!(*inst))
    {
        return AXIS2_FAILURE;
    }

    return AXIS2_SUCCESS;
}

AXIS2_EXPORT int 
axis2_remove_instance(axis2_svc_skeleton_t *inst,
                      const axutil_env_t *env)
{
	axis2_status_t status = AXIS2_FAILURE;

   if (inst)
   {
        status = AXIS2_SVC_SKELETON_FREE(inst, env);
    }
    return status;
}
