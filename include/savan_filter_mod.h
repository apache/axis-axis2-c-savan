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
 
#ifndef SAVAN_FILTER_H
#define SAVAN_FILTER_H

/**
  * @file savan_filter.h
  * @brief 
  */
#include <platforms/axutil_platform_auto_sense.h>
#include <axutil_utils_defines.h>
#include <axutil_env.h>
#include <axis2_conf.h>
#include <savan_subscriber.h>
#include <axiom_node.h>

#ifdef __cplusplus
extern "C"
{
#endif

/** 
 * @ingroup savan_filter
 * @{
 */
 
typedef struct savan_filter savan_filter_t;
typedef struct savan_filter_ops savan_filter_ops_t;

 /**
 * @brief Filter ops struct
 * Encapsulator struct for ops of savan_filter
 */
AXIS2_DECLARE_DATA struct savan_filter_ops
{ 
    void (AXIS2_CALL * 
            free)(
                savan_filter_t *filter,
                const axutil_env_t *env);

    axiom_node_t *(AXIS2_CALL *
            apply)(
                savan_filter_t *filter, 
                const axutil_env_t *env,
                savan_subscriber_t *subscriber,
                axiom_node_t *payload);


};

AXIS2_DECLARE_DATA struct savan_filter
{
    const savan_filter_ops_t *ops;
};


/**
 * Create the savan filter.
 * @param env environment object
 * @param conf axis2 configuration
 * @return status of the operation
 */
AXIS2_EXTERN savan_filter_t * AXIS2_CALL
savan_filter_create(
    const axutil_env_t *env,
    axis2_conf_t *conf);

/**
 * Deallocate the filter.
 * @param filter
 * @param env environment object
 */
void AXIS2_CALL 
savan_filter_free(
    savan_filter_t *filter,
    const axutil_env_t *envv);

/**
 * Apply filter to payload.
 * @param filter
 * @param env environment object
 * @param subscriber subscriber instant
 * @param payload payload to which the filter is applied
 * @return filtered payload
 */
AXIS2_EXTERN axiom_node_t *AXIS2_CALL
savan_filter_apply(
    savan_filter_t *filter, 
    const axutil_env_t *env,
    savan_subscriber_t *subscriber,
    axiom_node_t *payload);

/** @} */
#ifdef __cplusplus
}
#endif

#endif /*SAVAN_FILTER_H*/
