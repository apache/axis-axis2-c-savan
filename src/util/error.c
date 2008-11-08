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

#include <stdlib.h>
#include <savan_error.h>
#include <axutil_error_default.h>

AXIS2_IMPORT extern const axis2_char_t* axutil_error_messages[];

axis2_status_t AXIS2_CALL
savan_error_init()
{
    axutil_error_messages[SAVAN_ERROR_REQUESTED_DELIVERY_MODE_NOT_SUPPORTED] = 
        "Requested delivery mode is not supported";

    return AXIS2_SUCCESS;
}

