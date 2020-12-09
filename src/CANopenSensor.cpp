/*
 Copyright (C) 2015-2020 IoT.bzh Company

 Author: Johann Gautier <johann.gautier@iot.bzh>

 $RP_BEGIN_LICENSE$
 Commercial License Usage
  Licensees holding valid commercial IoT.bzh licenses may use this file in
  accordance with the commercial license agreement provided with the
  Software or, alternatively, in accordance with the terms contained in
  a written agreement between you and The IoT.bzh Company. For licensing terms
  and conditions see https://www.iot.bzh/terms-conditions. For further
  information use the contact form at https://www.iot.bzh/contact.

 GNU General Public License Usage
  Alternatively, this file may be used under the terms of the GNU General
  Public license version 3. This license is as published by the Free Software
  Foundation and appearing in the file LICENSE.GPLv3 included in the packaging
  of this file. Please review the following information to ensure the GNU
  General Public License requirements will be met
  https://www.gnu.org/licenses/gpl-3.0.html.
 $RP_END_LICENSE$
*/

#include <iostream>
#include <strings.h>


// if 2 before 1 => conflict with 'is_error' betwin a lely function and a json define named identically
#include "CANopenSlaveDriver.hpp" /*1*/
#include "CANopenSensor.hpp" /*2*/
#include "CANopenEncoder.hpp"
#include "CANopenGlue.hpp"

#ifndef ERROR
    #define ERROR -1
#endif

static void sensorDynRequest(afb_req_t request){
    // retrieve action handle from request and execute the request
    json_object *queryJ = afb_req_json(request);
    CANopenSensor* sensor = (CANopenSensor*) afb_req_get_vcbdata(request);
    sensor->request(request, queryJ);
}

CANopenSensor::CANopenSensor(afb_api_t api, json_object * sensorJ, CANopenSlaveDriver * slaveDriver)
{
    int err = 0;
    int idx;
    const char *type=NULL;
    const char *privilege=NULL;
    afb_auth_t *authent=NULL;
    json_object * regJ;
    json_object *argsJ=NULL;
    char* sensorVerb;

    // should already be allocated
    assert (sensorJ);

    // set default values
    m_slave = slaveDriver;
    m_api = api;
    m_sample = nullptr;

    err = wrap_json_unpack(sensorJ, "{ss,ss,so,ss,si,s?s,s?s,s?o,s?o !}",
                "uid", &m_uid,
                "type", &type,
                "register", &regJ,
                "format", &m_format,
                "size", &m_size,
                "info", &m_info,
                "privilege", &privilege,
                "args", &argsJ,
                "sample", &m_sample);
    if (err) {
        AFB_API_ERROR(m_api, "CANopenSensor: Fail to parse sensor: %s", json_object_to_json_string(sensorJ));
        return;
    }

    // Get sensor register and sub register from the parsed register
    try{
        idx = get_data_int(regJ);
    }catch(std::runtime_error& e){
        AFB_API_ERROR(m_api, "CANopenSensor: %s error at register convertion\n what() %s: ", m_uid, e.what());
        return;
    }
    m_register = ((uint32_t)idx & 0x00ffff00)>>8;
    m_subRegister = (uint32_t)idx & 0x000000ff;

    // create autentification for sensor
    if (privilege) {
       authent= (afb_auth_t*) calloc(1, sizeof (afb_auth_t));
       authent->type = afb_auth_Permission;
       authent->text = privilege;
    }

    // load Encoder
    CtlConfigT* ctrlConfig = (CtlConfigT*)afb_api_get_userdata(m_api);
    CANopenEncoder* coEncoder = (CANopenEncoder*)ctrlConfig->external;

    // Get the apropriet read/write callbacks
    try{
        m_function = coEncoder->getfunctionCB(type, m_size);
    }catch(std::out_of_range&){
        AFB_API_ERROR(m_api, "CANopenSensor: could not find sensor type %s size %d", type, m_size);
        return;
    }

    // Get the encode formater
    if (m_function.writeCB){
        try{
            m_encode = coEncoder->getEncodeFormateurCB(m_format);
        }catch(std::out_of_range&){
            AFB_API_ERROR(m_api, "CANopenSensor: could not find sensor encode formater %s", m_format);
            return;
        }
    }

    // Get the decode Formater
    if (m_function.readCB){
        try{
            m_decode = coEncoder->getDecodeFormateurCB(m_format);
        }catch(std::out_of_range&){
            AFB_API_ERROR(m_api, "CANopenSensor: could not find sensor decode formater %s", m_format);
            return;
        }
    }

    // if sensor uses SDO communication then it is asynchronus
    if(!strcasecmp(type, "SDO")) m_asyncSensor = true;

    m_currentVal.tDouble = 0;
    
    // create the verb for the sensor
    err = asprintf (&sensorVerb, "%s/%s", m_slave->uid(), m_uid);
    err = afb_api_add_verb(api, sensorVerb, m_info, sensorDynRequest, this, authent, 0, 0);
    if (err) {
        AFB_API_ERROR(api, "CANopenSensor : fail to register API verb=%s", sensorVerb);
        return;
    }
}

void CANopenSensor::request (afb_req_t request, json_object * queryJ) {

    char *action;
    json_object *dataJ = nullptr;
    json_object *responseJ = nullptr;
    int err;

    // parse request
    err= wrap_json_unpack(queryJ, "{ss s?o !}",
        "action", &action,
        "data", &dataJ
    );

    if (err) {
        afb_req_fail_f(request, "query-error", "CANopenSensor::request: invalid 'json' rtu=%s sensor=%s query=%s", m_slave->uid(), m_uid, json_object_get_string(queryJ));
        return;
    }

    if (!strcasecmp (action, "WRITE")) {
        if(!m_function.writeCB || !m_encode){
            afb_req_fail_f (request, "Write-error", "CANopenSensor::request: No write function available for %s : %s", m_slave->uid(), m_uid); 
            return;
        }
        err = write(dataJ);
        if(err){
            afb_req_fail_f (request, "Write-error", "CANopenSensor::request: Fail to write on sensor %s : %s", m_slave->uid(), m_uid); 
            return;
        }
    }
    else if (!strcasecmp (action, "READ")) {
        if(!m_function.readCB || !m_decode){
            afb_req_fail_f (request, "read-error", "CANopenSensor::request: No read function available for %s : %s", m_slave->uid(), m_uid); 
            return;
        }
        if(m_asyncSensor){
            afb_req_t current_req = request;
            afb_req_addref(current_req);
            m_slave->Post([this, request]() {
                json_object *responseJ;
                int err = read(&responseJ);
                if(err){
                    afb_req_fail_f (request, "read-error", "CANopenSensor::request: Fail to read sensor %s : %s", m_slave->uid(), m_uid); 
                    return;
                }
                afb_req_success(request, responseJ, NULL);
                afb_req_unref(request);
            });
            return;
        }
        else{
            int err = read(&responseJ);
            if(err){
                afb_req_fail_f (request, "read-error", "CANopenSensor::request: Fail to read sensor %s : %s", m_slave->uid(), m_uid); 
                return;
            }   
        }
    }
    else if (!strcasecmp (action, "SUBSCRIBE")) {
        if(!m_function.readCB || !m_decode){
            afb_req_fail_f (request, "subscribe-error","CANopenSensor::request: sensor '%s' is not readable", m_uid);  
            return;
        }
        // Use "Post" to avoid asynchronous conflicts
        afb_req_t current_req = request;
        afb_req_addref(current_req);
        m_slave->Post([this, request]() {
            json_object *responseJ;
            if (!m_event) {
                m_event = afb_api_make_event(m_api, m_uid);
                if (!m_event) {
                    afb_req_fail_f (request, "subscribe-error","CANopenSensor::request: fail to create event slave=%s sensor=%s", m_slave->uid(), m_uid);  
                    return;
                }
                int err = m_slave->addSensorEvent(this);
                if(err){
                    afb_req_fail_f (request, "subscribe-error","CANopenSensor::request: fail to add event slave=%s sensor=%s to event list", m_slave->uid(), m_uid);  
                    return;
                }
                err = afb_req_subscribe(request, m_event); 
                if (err){
                    afb_req_fail_f (request, "subscribe-error","CANopenSensor::request: fail to subscribe slave=%s sensor=%s", m_slave->uid(), m_uid);
                    return;
                }
                char * answer;
                asprintf(&answer,"Subscribe success on sensor %s/%s", m_slave->uid(), m_uid);
                responseJ = json_object_new_string(answer);
            }else{
                char * answer;
                asprintf(&answer,"sensor %s/%s alrady subscribed", m_slave->uid(), m_uid);
                responseJ = json_object_new_string(answer);
            }
            afb_req_success(request, responseJ, NULL);
            afb_req_unref(request);
        });
        return;
    }

    else if (!strcasecmp (action, "UNSUBSCRIBE")) {
        // Use "Post" to avoid asynchronous conflicts
        afb_req_t current_req = request;
        afb_req_addref(current_req);
        m_slave->Post([this, request]() {
            json_object *responseJ;
            if (m_event) {
                int err = afb_req_unsubscribe(request, m_event);
                if (err) {
                    afb_req_fail_f (request, "subscribe-error","CANopenSensor::request: fail to unsubscribe slave=%s sensor=%s", m_slave->uid(), m_uid); 
                    return;
                }
                err = m_slave->delSensorEvent(this);
                if (err){
                    afb_req_fail_f (request, "subscribe-error","CANopenSensor::request: fail to remove slave=%s sensor=%s from the subscribed list", m_slave->uid(), m_uid); 
                    return;
                }
                char * answer;
                asprintf(&answer,"sensor %s/%s successfully unsubscribed", m_slave->uid(), m_uid);
                responseJ = json_object_new_string(answer);
                m_event = nullptr;
            }
            else{
                char * answer;
                asprintf(&answer,"sensor %s/%s is not in the subscribed list", m_slave->uid(), m_uid);
                responseJ = json_object_new_string(answer);
            }
            afb_req_success(request, responseJ, NULL);
            afb_req_unref(request);
        });
        return;
    }
    else {
        afb_req_fail_f (request, "syntax-error", "CANopenSensor::request: action='%s' UNKNOWN rtu=%s sensor=%s query=%s", action, m_slave->uid(), m_uid, json_object_get_string(queryJ));
        return; 
    }
    // everything looks good let's respond
    afb_req_success(request, responseJ, NULL);
    return;
}

int CANopenSensor::read(json_object ** responseJ){
    if(!m_function.readCB || !m_decode) return ERROR;
    m_currentVal = m_function.readCB(this);
    *responseJ = m_decode(m_currentVal, this);
    return 0;
}

int CANopenSensor::write(json_object *output){
    if(!m_function.writeCB || !m_encode) return ERROR;
    m_currentVal = m_encode(output, this);
    m_function.writeCB(this, m_currentVal);
    return 0;
}

const char * CANopenSensor::info(){
    char * formatedInfo;
    bool first = true;
    asprintf(&formatedInfo, "%s/%s [", m_slave->uid(), m_uid);
    if(m_function.readCB){
        asprintf(&formatedInfo, "%sREAD", formatedInfo);
        first = false;
    }
    if(m_function.writeCB){
        if(!first) asprintf(&formatedInfo, "%s|", formatedInfo);
        asprintf(&formatedInfo, "%sWRITE",formatedInfo);
        first = false;
    }
    if(!first) asprintf(&formatedInfo, "%s|", formatedInfo);
    asprintf(&formatedInfo, "%sSUBSCRIBE|UNSUBSCRIBE] info: '%s'",formatedInfo, m_info);

    return formatedInfo;
}

json_object * CANopenSensor::infoJ(){
    json_object * sensor_info, *usage, *actions;
    char *verb;
    asprintf(&verb, "%s/%s", m_slave->uid(), m_uid);
    actions = json_object_new_array();
    usage = json_object_new_array();
    if(m_function.readCB)
        json_object_array_add(actions, json_object_new_string("read"));

    if(m_function.writeCB)
        json_object_array_add(actions, json_object_new_string("write"));

    json_object_array_add(actions, json_object_new_string("subscribe"));
    json_object_array_add(actions, json_object_new_string("unsubscribe"));

    wrap_json_pack(&usage, "{so ss}",
                        "action", actions,
                        "data", m_format
                    );


    wrap_json_pack(&sensor_info, "{ss ss* ss* so* sO*}",
                                "uid", m_uid,
                                "info", info(),
                                "verb", verb,
                                "usage", usage,
                                "sample", m_sample
                            );

    return sensor_info;
}