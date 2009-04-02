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
 
#include <savan_publisher_mod.h>
#include <axutil_log.h>
#include <axutil_hash.h>
#include <axutil_property.h>
#include <axutil_types.h>
#include <axutil_file_handler.h>
#include <platforms/axutil_platform_auto_sense.h>
#include <savan_constants.h>
#include <savan_util.h>
#include <savan_error.h>
#include <libxslt/xsltutils.h>
#include <axiom_soap.h>
#include <axiom_soap_const.h>
#include <axiom_soap_envelope.h>
#include <axiom_element.h>
#include <axiom_node.h>

/**
 *
 */
/** 
 * @brief Savan XPath Publisher Struct Impl
 *   Savan XPath Publisher 
 */
typedef struct savan_esb_publisher_mod
{
    savan_publisher_mod_t publishermod;
    axis2_conf_t *conf;
} savan_esb_publisher_mod_t;

#define SAVAN_INTF_TO_IMPL(publishermod) ((savan_esb_publisher_mod_t *) publishermod)

AXIS2_EXTERN void AXIS2_CALL
savan_esb_publisher_mod_free(
    savan_publisher_mod_t *publishermod,
    const axutil_env_t *env);

AXIS2_EXTERN void AXIS2_CALL
savan_esb_publisher_mod_publish(
    savan_publisher_mod_t *publishermod,
    const axutil_env_t *env,
    axis2_msg_ctx_t *msg_ctx);

static const savan_publisher_mod_ops_t savan_publisher_mod_ops = 
{
    savan_esb_publisher_mod_free,
    savan_esb_publisher_mod_publish
};

AXIS2_EXTERN savan_publisher_mod_t * AXIS2_CALL
savan_publisher_mod_create(
    const axutil_env_t *env,
    axis2_conf_t *conf)
{
    savan_esb_publisher_mod_t *publishermodimpl = NULL;
    axis2_char_t *publisher_template_path = NULL;
    
    publishermodimpl = AXIS2_MALLOC(env->allocator, sizeof(savan_esb_publisher_mod_t));
    if (!publishermodimpl)
    {
        AXIS2_HANDLE_ERROR(env, SAVAN_ERROR_FILTER_CREATION_FAILED, AXIS2_FAILURE);
        return NULL;
    }

    memset ((void *) publishermodimpl, 0, sizeof(savan_esb_publisher_mod_t));

    publisher_template_path = savan_util_get_module_param(env, conf, SAVAN_FILTER_TEMPLATE_PATH);
    if(!publisher_template_path)
    {
        savan_esb_publisher_mod_free((savan_publisher_mod_t *) publishermodimpl, env);
        AXIS2_LOG_ERROR(env->log, AXIS2_LOG_SI, "[savan] Publisher template path not set");
        return NULL;
    }

    publishermodimpl->publisher_template_path = publisher_template_path;

    publishermodimpl->dialect = NULL;
    publishermodimpl->conf = conf;
    publishermodimpl->publishermod.ops = &savan_publisher_mod_ops;

    return (savan_publisher_mod_t *) publishermodimpl;
}

AXIS2_EXTERN void AXIS2_CALL
savan_esb_publisher_mod_free(
    savan_publisher_mod_t *publishermod,
    const axutil_env_t *env)
{
    savan_esb_publisher_mod_t *publishermodimpl = NULL;
    publishermodimpl = SAVAN_INTF_TO_IMPL(publishermod);

    AXIS2_LOG_TRACE(env->log, AXIS2_LOG_SI, "[savan] Entry:savan_esb_publisher_mod_free");

    if(publishermodimpl->dialect)
    {
        AXIS2_FREE(env->allocator, publishermodimpl->dialect);
        publishermodimpl->dialect = NULL;
    }

    publishermodimpl->conf = NULL;

    if(publishermodimpl)
    {
        AXIS2_FREE(env->allocator, publishermodimpl);
        publishermodimpl = NULL;
    }

    AXIS2_LOG_TRACE(env->log, AXIS2_LOG_SI, "[savan] Exit:savan_esb_publisher_mod_free");
}

AXIS2_EXTERN void AXIS2_CALL
savan_esb_publisher_mod_publish(
    savan_publisher_mod_t *publishermod,
    const axutil_env_t *env,
    axis2_msg_ctx_t *msg_ctx)
{
    savan_esb_publisher_mod_t *publishermodimpl = NULL;

    publishermodimpl = SAVAN_INTF_TO_IMPL(publishermod);

    AXIS2_LOG_TRACE(env->log, AXIS2_LOG_SI, "[savan] Entry:savan_esb_publisher_mod_publish");

    AXIS2_LOG_TRACE(env->log, AXIS2_LOG_SI, "[savan] Exit:savan_esb_publisher_mod_publish");
    return AXIS2_FALSE;
}

