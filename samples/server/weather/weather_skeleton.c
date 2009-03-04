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
start_weather_thread(
    const axutil_env_t *env,
    axis2_conf_t *conf);

static void
stop_weather_thread(
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

static void * AXIS2_THREAD_FUNC
weather_worker_func(
    axutil_thread_t *thrd,
    void* data);

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

    axutil_array_list_add(svc_skeleton->func_array, env, "start");
    axutil_array_list_add(svc_skeleton->func_array, env, "stop");

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
    start_weather_thread(env, conf); 
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
        if (axiom_node_get_node_type(node, env) == AXIOM_ELEMENT)
        {
            axiom_element_t *element = NULL;
            element =
                (axiom_element_t *) axiom_node_get_data_element(node, env);
            if (element)
            {
                axis2_char_t *op_name =
                    axiom_element_get_localname(element, env);
                if (op_name)
                {
                    if (axutil_strcmp(op_name, "start") == 0)
                    {
                        start_weather_thread(env, conf); 
                    }
                    if (axutil_strcmp(op_name, "stop") == 0)
                    {
                        stop_weather_thread(env, conf); 
                    }
                }
            }
        }
    }

    printf("Weather service ERROR: invalid OM parameters in request\n");

    return NULL;
}

static void
start_weather_thread(
    const axutil_env_t *env,
    axis2_conf_t *conf)
{

	axutil_thread_t *worker_thread = NULL;
	weather_data_t *data = NULL;

    /* Invoke the business logic.
     * Depending on the function name invoke the correct impl method.
     */

    data = AXIS2_MALLOC(env->allocator, sizeof(weather_data_t));
    data->env = (axutil_env_t*)env;
    data->conf = conf;
    
    worker_thread = axutil_thread_pool_get_thread(env->thread_pool,
        weather_worker_func, (void*)data);
    if(! worker_thread)
    {
        return;
    }
    axutil_thread_pool_thread_detach(env->thread_pool, worker_thread);
}

static void
stop_weather_thread(
    const axutil_env_t *env,
    axis2_conf_t *conf)
{
    axis2_svc_t *svc = NULL;
    axutil_param_t *param = NULL;
    
    svc = axis2_conf_get_svc(conf, env, WEATHER);
    param = axis2_svc_get_param(svc, env, WEATHER_STATUS);
    if(param)
    {
        axutil_param_set_value(param, env, AXIS2_VALUE_FALSE);
    }
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
    error_ele = axiom_element_create(env, node, "TestServiceError", NULL, 
        &error_node);
    axiom_element_set_text(error_ele, env, "Test service failed ", 
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

static void * AXIS2_THREAD_FUNC
weather_worker_func(
    axutil_thread_t *thrd,
    void* data)
{
    axutil_env_t *main_env = NULL;
    axutil_env_t *env = NULL;
    axiom_namespace_t *test_ns = NULL;
    axiom_element_t* test_elem = NULL;
    axiom_node_t *test_node = NULL;
    axiom_element_t* test_elem1 = NULL;
    axiom_node_t *test_node1 = NULL;
    axis2_conf_t *conf = NULL;
    axis2_svc_t *svc = NULL;
    axis2_char_t *value = AXIS2_VALUE_TRUE;
    axutil_param_t *param = NULL;
    
    weather_data_t *mydata = (weather_data_t*)data;
    main_env = mydata->env;
    conf = mydata->conf;
    
    env = axutil_init_thread_env(main_env);

    svc = axis2_conf_get_svc(conf, env, WEATHER);
    param = axutil_param_create(env, WEATHER_STATUS, AXIS2_VALUE_TRUE);
    if(!param)
    {
        AXIS2_HANDLE_ERROR(env, AXIS2_ERROR_NO_MEMORY, AXIS2_FAILURE);
        return NULL;
    }
    axis2_svc_add_param(svc, env, param);
    while(axutil_strcmp(value, AXIS2_VALUE_TRUE))
    {
        AXIS2_LOG_DEBUG(env->log, AXIS2_LOG_SI, "[savan] Inside while loop");
        {
            savan_publishing_client_t *pub_client = NULL;

            pub_client = savan_publishing_client_create(env, conf, svc);
            /* Build a payload and pass it to the savan publishing client */ 
            test_ns = axiom_namespace_create (env, "http://www.wso2.com/savan/c/weather", "weather");
            test_elem = axiom_element_create(env, NULL, "weather", test_ns, &test_node);
            test_elem1 = axiom_element_create(env, test_node, "weather_report", NULL, &test_node1);

			axiom_element_set_text(test_elem1, env, "sunny day", test_node1);

            /*savan_publishing_client_publish(pub_client, env, test_node, topic);*/
            savan_publishing_client_free(pub_client, env);
            param = axis2_svc_get_param(svc, env, WEATHER_STATUS);
            if(param)
            {
                value = axutil_param_get_value(param, env);
            }
        }
        AXIS2_SLEEP(5);
    }
    return NULL;
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
