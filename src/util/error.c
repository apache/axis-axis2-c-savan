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

    axutil_error_messages[SAVAN_ERROR_EXPIRATION_TIME_REQUESTED_IS_INVALID] = 
        "Expiration time requested is invalid";

    axutil_error_messages[SAVAN_ERROR_ONLY_EXPIRATION_DURATIONS_ARE_SUPPORTED] = 
        "Only expiration durations are supported";

    axutil_error_messages[SAVAN_ERROR_FILTERING_IS_NOT_SUPPORTED] = 
        "Filtering is not supported";

    axutil_error_messages[SAVAN_ERROR_REQUESTED_FILTER_DIALECT_IS_NOT_SUPPORTED] = 
        "Requested filter dialect is not supported";

    axutil_error_messages[SAVAN_ERROR_MESSAGE_IS_NOT_VALID_AND_CANNOT_BE_PROCESSED] = 
        "Messsage is not valid and cannot be processed";

    axutil_error_messages[SAVAN_ERROR_MESSAGE_CANNOT_BE_PROCESSED_BY_EVENT_SOURCE] = 
        "Message cannot be processed by the event source";
    
    axutil_error_messages[SAVAN_ERROR_UNABLE_TO_RENEW] = 
        "Unable to Renew";
    
    axutil_error_messages[SAVAN_ERROR_SUBSCRIBER_NOT_FOUND] = 
        "Subscriber is not found";
        
    axutil_error_messages[SAVAN_ERROR_COULD_NOT_POPULATE_TOPIC] = 
        "Could not populate Topic";
    
    axutil_error_messages[SAVAN_ERROR_PARSING_SUBSCRIBER_NODE_FAILED] = 
        "Parsing subsriber node failed";
    
    axutil_error_messages[SAVAN_ERROR_APPLYING_FILTER_FAILED] = 
        "Applying filter failed";

    return AXIS2_SUCCESS;
}

