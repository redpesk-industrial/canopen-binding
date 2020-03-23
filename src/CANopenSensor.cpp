#include <iostream>
#include <strings.h>


// if 2 before 1 => conflict with 'is_error' betwin a lely function and a json define named identically
#include "CANopenSlaveDriver.hpp" /*1*/
#include "CANopenSensor.hpp" /*2*/
#include "CANopen-encoder.hpp"


#ifndef ERROR
    #define ERROR -1
#endif

// #define CO_TYPE_SDO 1
// #define CO_TYPE_TPDO 2
// #define CO_TYPE_RPDO 3

static void sensorDynRequest(afb_req_t request){
    // retrieve action handle from request and execute the request
    json_object *queryJ = afb_req_json(request);
    CANopenSensor* sensor = (CANopenSensor*) afb_req_get_vcbdata(request);
    sensor->request(request, queryJ);
}

CANopenSensor::CANopenSensor(afb_api_t api, json_object * sensorJ, CANopenSlaveDriver * slaveDriver)
{
    int err = 0;
    int sensorRegister;
    const char *type=NULL;
    const char *format=NULL;
    const char *privilege=NULL;
    afb_auth_t *authent=NULL;
    json_object *argsJ=NULL;
    char* sensorVerb;
    
    // should already be allocated
    assert (sensorJ);

    // set default values
    m_slave = slaveDriver;
    m_api = api;

    err = wrap_json_unpack(sensorJ, "{ss,ss,si,s?s,s?s,s?s,s?o !}",
                "uid", &m_uid,
                "type", &type,
                "register", &sensorRegister,
                "format", &format,
                "info", &m_info,
                "privilege", &privilege,
                "args", &argsJ);
    if (err) {
        AFB_API_ERROR(api, "CANopenSensor: Fail to parse sensor: %s", json_object_to_json_string(sensorJ));
        return;
    }

    // Get sensor register and sub register from the parsed register
    m_register = ((uint32_t)sensorRegister & 0x00ffff00)>>8;
    m_subRegister = (uint32_t)sensorRegister & 0x000000ff;

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
        m_function = coEncoder->functionCB(type, format);
    }catch(std::out_of_range){
        AFB_API_ERROR(m_api, "CANopenSensor: could not find sensor type %s format %s", type, format);
        return;
    }
    // try{
    //     m_type = AvalableTypes.at(type);
    // } catch(std::out_of_range){
    //     AFB_API_ERROR(api, "CANopenSensor: could not find sensor type %s", type);
    //     return;
    // }

    // // Get the funchion callbacks for reading and writing on the sensor
    // try{
    //     m_function = encodingTable.at(m_type).at(format);
    // } catch(std::out_of_range){
    //     AFB_API_ERROR(api, "CANopenSensor: could not find sensor format %s", format);
    //     return;
    // }

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
        afb_req_fail_f(
            request,
            "query-error",
            "CANopenSensor::request: invalid 'json' rtu=%s sensor=%s query=%s",
            m_slave->uid(), m_uid, json_object_get_string(queryJ)
        );
        return;
    }

    if (!strcasecmp (action, "WRITE")) {
        if(!m_function.writeCB){
            afb_req_fail_f (request, "Write-error", "CANopenSensor::request: No write function avalable for %s : %s"
            , m_slave->uid(), m_uid); 
            return;
        }
        write(dataJ);

    }
    else if (!strcasecmp (action, "READ")) {
        if(!m_function.readCB){
            afb_req_fail_f (request, "read-error", "CANopenSensor::request: No read function avalable for %s : %s"
            , m_slave->uid(), m_uid); 
            return;
        }
        if(m_asyncSensor){
            afb_req_t current_req = request;
            afb_req_addref(current_req);
            m_slave->Post([this, request]() {
                json_object *responseJ;
                m_function.readCB(this, &responseJ);
                std::cout << "DEBUG : Async read of slave : " << m_slave->id() << " [" << std::hex << m_register << "]:[" << m_subRegister << "] returned " << json_object_get_string(responseJ) << std::endl;
                afb_req_success(request, responseJ, NULL);
                afb_req_unref(request);
            });
            return;
        }
        else{
            m_function.readCB(this, &responseJ);
        }
    } 
    else if (!strcasecmp (action, "SUBSCRIBE")) {
        err = eventCreate();
        if (err) return;
        err = afb_req_subscribe(request, m_event); 
        if (err){
            afb_req_fail_f (request, "subscribe-error","CANopenSensor::request: fail to subscribe slave=%s sensor=%s"
            , m_slave->uid(), m_uid);
            return;
        }
        //m_slave->rpdo_mapped[m_register][m_subRegister];
    }
    else if (!strcasecmp (action, "UNSUBSCRIBE")) {
        if (m_event) {
            err=afb_req_unsubscribe(request, m_event);
            if (err) {
                afb_req_fail_f (request, "subscribe-error","CANopenSensor::request: fail to unsubscribe slave=%s sensor=%s"
                , m_slave->uid(), m_uid); 
                return;
            }
            m_slave->delSensorEvent(this);
        }
    }
    else {
        afb_req_fail_f (request, "syntax-error", "CANopenSensor::request: action='%s' UNKNOWN rtu=%s sensor=%s query=%s"
            , action, m_slave->uid(), m_uid, json_object_get_string(queryJ));
        return; 
    }
    // everything looks good let's responde
    afb_req_success(request, responseJ, NULL);
    return;
}

int CANopenSensor::read(json_object **responseJ){
    if(!m_function.readCB) return ERROR;
    m_function.readCB(this, responseJ);
    return 0;
}

int CANopenSensor::write(json_object *output){
    if(!m_function.writeCB) return ERROR;
    m_function.writeCB(this, output);
    return 0;
}

// template <typename T> void CANopenSensor::sdoWrite(T val){
//     m_slave->AsyncWrite(m_register, m_subRegister, std::forward<T> val);
// }
// template <typename T> T CANopenSensor::sdoRead(){
//     return m_slave->AsyncWrite(m_register, m_subRegister);
// }
// template <typename T> void CANopenSensor::pdoWrite(T){
//     m_slave->tpdo_mapped[m_register][m_subRegister] = T;
// }
// template <typename T> T CANopenSensor::pdoRead(){
//     return m_slave->rpdo_mapped[m_register][m_subRegister];
// }

int CANopenSensor::eventCreate(){
    if(!m_function.readCB){
        AFB_API_ERROR(m_api, "CANopenSensor::eventCreate: sensor '%s' is not readable", m_uid);  
        return ERROR;
    }
    if (!m_event) {
        m_event = afb_api_make_event(m_api, m_uid);
        if (!m_event) {
            AFB_API_ERROR(m_api, "CANopenSensor::eventCreate: fail to create event slave=%s sensor=%s", m_slave->uid(), m_uid);  
            return ERROR;
        }
    }
    m_slave->addSensorEvent(this);
    return 0;
}

// const std::map<std::string, CANopenSensor::CANopenFunctionCbS> CANopenSensor::m_SDOfunctionCBs{
//     {"uint8", {coSDOreadUint8, coSDOwriteUint8}},
//     {"uint16",{coSDOreadUint16, coSDOwriteUint16}},
//     {"uint32",{coSDOreadUint32, coSDOwriteUint32}},
// };

// const std::map<std::string, CANopenSensor::CANopenFunctionCbS> CANopenSensor::m_RPDOfunctionCBs{
//     {"uint8", {coPDOreadUint8, nullptr}},
//     {"uint16",{coPDOreadUint16, nullptr}},
//     {"uint32",{coPDOreadUint32, nullptr}},
// };

// const std::map<std::string, CANopenSensor::CANopenFunctionCbS> CANopenSensor::m_TPDOfunctionCBs{
//     {"uint8", {nullptr, coPDOwriteUint8}},
//     {"uint16",{nullptr, coPDOwriteUint16}},
//     {"uint32",{nullptr, coPDOwriteUint32}},
// };

// const std::map<std::string, uint> CANopenSensor::m_AvalableTypes{
//     {"SDO", CO_TYPE_SDO},
//     {"TPDO", CO_TYPE_TPDO},
//     {"RPDO", CO_TYPE_RPDO}
// };

// const std::map<uint, std::map<std::string, CANopenSensor::CANopenFunctionCbS>> CANopenSensor::m_encodingTable{
//     {CO_TYPE_SDO, CANopenSensor::m_SDOfunctionCBs},
//     {CO_TYPE_TPDO, CANopenSensor::m_TPDOfunctionCBs},
//     {CO_TYPE_RPDO, CANopenSensor::m_RPDOfunctionCBs}
// };

// int CANopenSensor::coSDOwriteUint8(CANopenSensor* sensor, json_object* inputJ){
//     int val = json_object_get_int(inputJ);
//     sensor->m_slave->AsyncWrite<uint8_t>(sensor->m_register, sensor->m_subRegister, (uint8_t)val);
//     return 0;
// }

// int CANopenSensor::coSDOwriteUint16(CANopenSensor* sensor, json_object* inputJ){
//     int val = json_object_get_int(inputJ);
//     sensor->m_slave->AsyncWrite<uint16_t>(sensor->m_register, sensor->m_subRegister, (uint16_t)val);
//     return 0;
// }

// int CANopenSensor::coSDOwriteUint32(CANopenSensor* sensor, json_object* inputJ){
//     int val = json_object_get_int(inputJ);
//     sensor->m_slave->AsyncWrite<uint32_t>(sensor->m_register, sensor->m_subRegister, (uint32_t)val);
//     return 0;
// }

// int CANopenSensor::coSDOreadUint8(CANopenSensor* sensor, json_object** responseJ){
//     int val = sensor->m_slave->Wait(sensor->m_slave->AsyncRead<uint8_t>(sensor->m_register, sensor->m_subRegister));
//     *responseJ = json_object_new_int(val);
//     return 0;
// }

// int CANopenSensor::coSDOreadUint16(CANopenSensor* sensor, json_object** responseJ){
//     int val = sensor->m_slave->Wait(sensor->m_slave->AsyncRead<uint16_t>(sensor->m_register, sensor->m_subRegister));
//     *responseJ = json_object_new_int(val);
//     return 0;
// }

// int CANopenSensor::coSDOreadUint32(CANopenSensor* sensor, json_object** responseJ){
//     int val = sensor->m_slave->Wait(sensor->m_slave->AsyncRead<uint32_t>(sensor->m_register, sensor->m_subRegister));
//     *responseJ = json_object_new_int(val);
//     return 0;
// }

// int CANopenSensor::coPDOwriteUint8(CANopenSensor* sensor, json_object* inputJ){
//     sensor->m_slave->tpdo_mapped[sensor->m_register][sensor->m_subRegister] = (uint8_t)json_object_get_int(inputJ);
//     return 0;
// }

// int CANopenSensor::coPDOwriteUint16(CANopenSensor* sensor, json_object* inputJ){
//     sensor->m_slave->tpdo_mapped[sensor->m_register][sensor->m_subRegister] = (uint16_t)json_object_get_int(inputJ);
//     return 0;
// }

// int CANopenSensor::coPDOwriteUint32(CANopenSensor* sensor, json_object* inputJ){
//     sensor->m_slave->tpdo_mapped[sensor->m_register][sensor->m_subRegister] = (uint32_t)json_object_get_int(inputJ);
//     return 0;
// }

// int CANopenSensor::coPDOreadUint8(CANopenSensor* sensor, json_object** outputJ){
//     uint8_t val = sensor->m_slave->rpdo_mapped[sensor->m_register][sensor->m_subRegister];
//     *outputJ = json_object_new_int(val);
//     return 0;
// }

// int CANopenSensor::coPDOreadUint16(CANopenSensor* sensor, json_object** outputJ){
//     uint16_t val = sensor->m_slave->rpdo_mapped[sensor->m_register][sensor->m_subRegister];
//     *outputJ = json_object_new_int(val);
//     return 0;
// }

// int CANopenSensor::coPDOreadUint32(CANopenSensor* sensor, json_object** outputJ){
//     uint32_t val = sensor->m_slave->rpdo_mapped[sensor->m_register][sensor->m_subRegister];
//     *outputJ = json_object_new_int(val);
//     return 0;
// }