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
 
#ifndef SAVAN_PUBLISHER_MOD_H
#define SAVAN_PUBLISHER_MOD_H

/**
  * @file savan_publisher_mod.h
  * @brief 
  */
#include <platforms/axutil_platform_auto_sense.h>
#include <axutil_utils_defines.h>
#include <axutil_env.h>
#include <axis2_conf.h>
#include <axis2_msg_ctx.h>
#include <axiom_node.h>
#include <savan_storage_mgr.h>

#ifdef __cplusplus
extern "C"
{
#endif

/** 
 * @ingroup savan_publisher_mod
 * @{
 */
 
typedef struct savan_publisher_mod savan_publisher_mod_t;
typedef struct savan_publisher_mod_ops savan_publisher_mod_ops_t;

 /**
 * @brief Publisher ops struct
 * Encapsulator struct for ops of savan_publisher_mod
 */
AXIS2_DECLARE_DATA struct savan_publisher_mod_ops
{ 
    void (AXIS2_CALL * 
            free)(
                savan_publisher_mod_t *publisher,
                const axutil_env_t *env);

    void (AXIS2_CALL *
            publish)(
                savan_publisher_mod_t *publisher, 
                const axutil_env_t *env,
                void *msg_ctx,
                savan_subs_mgr_t *subs_mgr);

};

AXIS2_DECLARE_DATA struct savan_publisher_mod
{
    const savan_publisher_mod_ops_t *ops;
};


/**
 * Create the savan publisher.
 * @param env environment object
 * @param conf axis2 configuration
 * @return status of the operation
 */
AXIS2_EXTERN savan_publisher_mod_t * AXIS2_CALL
savan_publisher_mod_create_with_conf(
    const axutil_env_t *env,
    axis2_conf_t *conf);

/**
 * Create the savan publisher.
 * @param env environment object
 * @return status of the operation
 */
AXIS2_EXTERN savan_publisher_mod_t * AXIS2_CALL
savan_publisher_mod_create(
    const axutil_env_t *env);

/**
 * Deallocate the publisher.
 * @param publisher
 * @param env environment object
 */
AXIS2_EXTERN void AXIS2_CALL 
savan_publisher_mod_free(
    savan_publisher_mod_t *publishermod,
    const axutil_env_t *env);

/**
 * Apply publisher to payload.
 * @param publisher
 * @param env environment object
 * @param msg_ctx Message context of the incoming event message.
 */
AXIS2_EXTERN void AXIS2_CALL
savan_publisher_mod_publish(
    savan_publisher_mod_t *publishermod, 
    const axutil_env_t *env,
    void *msg_ctx,
    savan_subs_mgr_t *subs_mgr);

/** @} */
#ifdef __cplusplus
}
#endif

#endif /*SAVAN_PUBLISHER_MOD_H*/
