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

#include <axiom_xml_writer.h>
#include <axiom_element.h>
#include <stdio.h>

#include "weather.h"

axiom_node_t *
build_om_programatically(
        const axutil_env_t *env, 
        axis2_char_t *text);

axiom_node_t *
axis2_weather_start(
        const axutil_env_t *env, 
        axiom_node_t *node)
{
	axiom_node_t *ret_node = NULL;

    ret_node = build_om_programatically(env, "Weather event source started successfully");

    return ret_node;
}

axiom_node_t *
axis2_weather_stop(
        const axutil_env_t *env, 
        axiom_node_t *node)
{
	axiom_node_t *ret_node = NULL;

    ret_node = build_om_programatically(env, "Weather event source stopped successfully");

    return ret_node;
}


/* Builds the response content */
axiom_node_t *
build_om_programatically(
        const axutil_env_t *env, 
        axis2_char_t *text)
{
    axiom_node_t *om_node = NULL;
    axiom_element_t* om_ele = NULL;
    axiom_node_t* text_om_node = NULL;
    axiom_element_t * text_om_ele = NULL;
    axiom_namespace_t *ns = NULL;
    
    ns = axiom_namespace_create (env, "http://ws.apache.org/axis2/c/savan/samples/weather", "ns");

    om_ele = axiom_element_create(env, NULL, "weather", ns, &om_node);
    
    text_om_ele = axiom_element_create(env, om_node, "text", NULL, &text_om_node);

    axiom_element_set_text(text_om_ele, env, text, text_om_node);
    
    return om_node;
}

