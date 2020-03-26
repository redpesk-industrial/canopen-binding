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
    const char *format=NULL;
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

    err = wrap_json_unpack(sensorJ, "{ss,ss,so,s?s,s?s,s?s,s?o !}",
                "uid", &m_uid,
                "type", &type,
                "register", &regJ,
                "format", &format,
                "info", &m_info,
                "privilege", &privilege,
                "args", &argsJ);
    if (err) {
        AFB_API_ERROR(api, "CANopenSensor: Fail to parse sensor: %s", json_object_to_json_string(sensorJ));
        return;
    }

    // Get sensor register and sub register from the parsed register
    try{
        idx = get_data_int(regJ);
    }catch(std::runtime_error& e){
        AFB_API_ERROR(api, "CANopenSensor: %s error at register convertion\n what() %s: ", m_uid, e.what());
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


    // check for an avalable sensor type
    CtlConfigT* ctrlConfig = (CtlConfigT*)afb_api_get_userdata(m_api);
    CANopenEncoder* coEncoder = (CANopenEncoder*)ctrlConfig->external;
    try{
        m_function = coEncoder->getfunctionCB(type, format);
    }catch(std::out_of_range&){
        AFB_API_ERROR(m_api, "CANopenSensor: could not find sensor type %s format %s", type, format);
        return;
    }

    if(!strcasecmp(type, "SDO")) m_asyncSensor = true;

    // create the verb for the sensor
    err = asprintf (&sensorVerb, "%s/%s", m_slave->prefix(), m_uid);
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
        if(!m_function.writeCB){
            afb_req_fail_f (request, "Write-error", "CANopenSensor::request: No write function available for %s : %s", m_slave->uid(), m_uid); 
            return;
        }
        err = m_function.writeCB(this, dataJ);
        if(err){
            afb_req_fail_f (request, "Write-error", "CANopenSensor::request: Fail to write on sensor %s : %s", m_slave->uid(), m_uid); 
            return;
        }
    }
    else if (!strcasecmp (action, "READ")) {
        if(!m_function.readCB){
            afb_req_fail_f (request, "read-error", "CANopenSensor::request: No read function available for %s : %s", m_slave->uid(), m_uid); 
            return;
        }
        if(m_asyncSensor){
            afb_req_t current_req = request;
            afb_req_addref(current_req);
            m_slave->Post([this, request]() {
                json_object *responseJ;
                int err = m_function.readCB(this, &responseJ);
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
            int err = m_function.readCB(this, &responseJ);
            if(err){
                afb_req_fail_f (request, "read-error", "CANopenSensor::request: Fail to read sensor %s : %s", m_slave->uid(), m_uid); 
                return;
            }   
        }
    }
    else if (!strcasecmp (action, "SUBSCRIBE")) {
        if(!m_function.readCB){
            afb_req_fail_f (request, "subscribe-error","CANopenSensor::request: sensor '%s' is not readable", m_uid);  
            return;
        }
        if (!m_event) {
            m_event = afb_api_make_event(m_api, m_uid);
            if (!m_event) {
                afb_req_fail_f (request, "subscribe-error","CANopenSensor::request: fail to create event slave=%s sensor=%s", m_slave->uid(), m_uid);  
                return;
            }
        }
        m_slave->addSensorEvent(this);
        err = afb_req_subscribe(request, m_event); 
        if (err){
            afb_req_fail_f (request, "subscribe-error","CANopenSensor::request: fail to subscribe slave=%s sensor=%s", m_slave->uid(), m_uid);
            return;
        }
        AFB_REQ_DEBUG(request, "Subscribe success on %s/%s register : [0x%x][0x%x]", m_slave->prefix(), m_uid, m_register, m_subRegister);
    }

    else if (!strcasecmp (action, "UNSUBSCRIBE")) {
        if (m_event) {
            err = afb_req_unsubscribe(request, m_event);
            if (err) {
                afb_req_fail_f (request, "subscribe-error","CANopenSensor::request: fail to unsubscribe slave=%s sensor=%s", m_slave->uid(), m_uid); 
                return;
            }
            m_slave->delSensorEvent(this);
        }
    }
    else {
        afb_req_fail_f (request, "syntax-error", "CANopenSensor::request: action='%s' UNKNOWN rtu=%s sensor=%s query=%s", action, m_slave->uid(), m_uid, json_object_get_string(queryJ));
        return; 
    }
    // everything looks good let's respond
    afb_req_success(request, responseJ, NULL);
    return;
}

int CANopenSensor::read(json_object **responseJ){
    if(!m_function.readCB) return ERROR;
    int err = m_function.readCB(this, responseJ);
    return err;
}

int CANopenSensor::write(json_object *output){
    if(!m_function.writeCB) return ERROR;
    int err = m_function.writeCB(this, output);
    return err;
}
