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
 
#include <savan_filter.h>
#include <axutil_log.h>
#include <axutil_hash.h>
#include <axutil_property.h>
#include <axutil_types.h>
#include <axutil_file_handler.h>
#include <platforms/axutil_platform_auto_sense.h>
#include <savan_constants.h>
#include <savan_util.h>
#include <savan_error.h>

/**
 *
 */
/** 
 * @brief Savan XPath Filter Struct Impl
 *   Savan XPath Filter 
 */
typedef struct savan_xpath_filter
{
    savan_filter_t filter;
    axis2_char_t *dialect;
    axis2_conf_t *conf;
} savan_xpath_filter_t;

#define SAVAN_INTF_TO_IMPL(filter) ((savan_xpath_filter_t *) filter)

AXIS2_EXTERN void AXIS2_CALL
savan_xpath_filter_free(
    savan_filter_t *filter,
    const axutil_env_t *env);

AXIS2_EXTERN axiom_node_t *AXIS2_CALL
savan_xpath_filter_apply(
    savan_filter_t *filter,
    const axutil_env_t *env,
    savan_subscriber_t *subscriber,
    axiom_node_t *payload);

static const savan_filter_ops_t savan_filter_ops = 
{
    savan_xpath_filter_free,
    savan_xpath_filter_apply
};

AXIS2_EXTERN savan_filter_t * AXIS2_CALL
savan_filter_create(
    const axutil_env_t *env,
    axis2_conf_t *conf)
{
    savan_xpath_filter_t *filterimpl = NULL;
    
    filterimpl = AXIS2_MALLOC(env->allocator, sizeof(savan_xpath_filter_t));
    if (!filterimpl)
    {
        AXIS2_HANDLE_ERROR(env, SAVAN_ERROR_FILTER_CREATION_FAILED, AXIS2_FAILURE);
        return NULL;
    }

    memset ((void *) filterimpl, 0, sizeof(savan_xpath_filter_t));

    filterimpl->dialect = NULL;
    filterimpl->conf = NULL;
    filterimpl->filter.ops = &savan_filter_ops;

    return (savan_filter_t *) filterimpl;
}

AXIS2_EXTERN void AXIS2_CALL
savan_xpath_filter_free(
    savan_filter_t *filter,
    const axutil_env_t *env)
{
    savan_xpath_filter_t *filterimpl = NULL;
    filterimpl = SAVAN_INTF_TO_IMPL(filter);

    AXIS2_LOG_TRACE(env->log, AXIS2_LOG_SI, "[savan] Entry:savan_xpath_filter_free");

    if(filterimpl->dialect)
    {
        AXIS2_FREE(env->allocator, filterimpl->dialect);
        filterimpl->dialect = NULL;
    }

    filterimpl->conf = NULL;

    if(filterimpl)
    {
        AXIS2_FREE(env->allocator, filterimpl);
        filterimpl = NULL;
    }

    AXIS2_LOG_TRACE(env->log, AXIS2_LOG_SI, "[savan] Exit:savan_xpath_filter_free");
}

AXIS2_EXTERN axiom_node_t *AXIS2_CALL
savan_xpath_filter_apply(
    savan_filter_t *filter,
    const axutil_env_t *env,
    savan_subscriber_t *subscriber,
    axiom_node_t *payload)
{
    savan_xpath_filter_t *filterimpl = NULL;
    filterimpl = SAVAN_INTF_TO_IMPL(filter);

    AXIS2_LOG_TRACE(env->log, AXIS2_LOG_SI, 
            "[savan] Entry:savan_xpath_filter_insert_subscriber");
	
    AXIS2_LOG_TRACE(env->log, AXIS2_LOG_SI, 
            "[savan] Exit:savan_xpath_filter_insert_subscriber");
    return NULL;
}

