// For debug
#include <iostream>

//String Compare
#include <strings.h>



// if 2 before 1 => conflict with 'is_error' betwin a lely function and a json define named identically
#include "CANopenSlaveDriver.hpp" /*1*/
#include "CANopenSensor.hpp" /*2*/

//json vraping
//#include <wrap-json.h>

#ifndef ERROR
    #define ERROR -1
#endif

#define CO_TYPE_SDO 1
#define CO_TYPE_TPDO 2
#define CO_TYPE_RPDO 3

static void sensorDynRequest(afb_req_t request){
    // retrieve action handle from request and execute the request
    json_object *queryJ = afb_req_json(request);
    CANopenSensor* sensor = (CANopenSensor*) afb_req_get_vcbdata(request);
    sensor->request(request, queryJ);
}

CANopenSensor::CANopenSensor(afb_api_t api, json_object * sensorJ, CANopenSlaveDriver * slaveDriver)
{
    std::cout << "création d'un sensor pour le driver de l'esclave " << (int)slaveDriver->id() << "\n";

    int err = 0;
    int sensorRegister;
    const char *type=NULL;
    const char *format=NULL;
    const char *privilege=NULL;
    //char * adminCmd;
    afb_auth_t *authent=NULL;
    json_object *argsJ=NULL;
    char* sensorVerb;
    //CANopenSourceT source;
    m_api = api;

    // should already be allocated
    assert (sensorJ);

    // set default values
    //memset(sensor, 0, sizeof (CANopenSensorT));
    m_slave = slaveDriver;
    //m_iddle = rtu->iddle;
    m_count = 1;

    // m_readCB = nullptr;
    // m_writeCB = nullptr;

    err = wrap_json_unpack(sensorJ, "{ss,ss,si,s?s,s?s,s?s,s?i,s?o !}",
                "uid", &m_uid,
                "type", &type,
                "register", &sensorRegister,
                "format", &format,
                "info", &m_info,
                "privilege", &privilege,
                //"iddle", &m_iddle,
                "count", &m_count,
                "args", &argsJ);
    if (err) {
        AFB_API_ERROR(api, "CANopenSensor: Fail to parse sensor: %s", json_object_to_json_string(sensorJ));
        return;
    }

    m_register = ((uint32_t)sensorRegister & 0x00ffff00)>>8;
    m_subRegister = (uint32_t)sensorRegister & 0x000000ff;
    //m_format = format

    // if not API prefix let's use RTU uid
    //if (!m_prefix) m_prefix= m_uid;

    // create autentification for sensor
    if (privilege) {
       authent= (afb_auth_t*) calloc(1, sizeof (afb_auth_t));
       authent->type = afb_auth_Permission;
       authent->text = privilege;
    }

    // //Find sensor Type
    // try{
    //     m_readCB = m_avalableReadCBs.at(format);
    // } catch(std::out_of_range){
    //     AFB_API_ERROR(api, "CANopenSensor: could not find sensor format %s", format);
    //     return;
    // }
    try{
        m_type = m_AvalableTypes.at(type);
    } catch(std::out_of_range){
        AFB_API_ERROR(api, "CANopenSensor: could not find sensor type %s", type);
        return;
    }

    try{
        m_function = m_SDOfunctionCBs.at(format);
    } catch(std::out_of_range){
        AFB_API_ERROR(api, "CANopenSensor: could not find sensor format %s", format);
        return;
    }

    //create the verb for the sensor
    err = asprintf (&sensorVerb, "%s/%s", m_slave->prefix(), m_uid);
    err = afb_api_add_verb(api, sensorVerb, m_info, sensorDynRequest, this, authent, 0, 0);
    if (err) {
        AFB_API_ERROR(api, "CANopenSensor : fail to register API verb=%s", sensorVerb);
        return;
    }

    std::cout << "DEBUG : sensor " << m_uid << " created : " << std::endl
              << "                register : " << std::dec << sensorRegister << " => [" << std::hex << (int)m_register << "][" << (int)m_subRegister << "]" << std::endl
              << "                verb : " << std::dec << sensorVerb << std::endl
              << "                info : " << std::dec << m_info << std::endl;
    return;  
}

void CANopenSensor::request (afb_req_t request, json_object * queryJ) {
    
    //CANopenRtuT *rtu = sensor->rtu;
    char *action;
    json_object *dataJ = nullptr;
    json_object *responseJ = nullptr;
    int err;

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
        /*if (!m_function->writeCB) goto OnWriteError;
        err = (m_function->writeCB) (this, dataJ);
        if (err) goto OnWriteError;*/
        // uint val = json_object_get_int(dataJ);
        // printf("DEBUG : writing %d on sensor [%x][%x]", val, m_register, m_subRegister);
        // m_slave->tpdo_mapped[m_register][m_subRegister] = val;
        if(!m_function.writeCB){
            afb_req_fail_f (request, "Write-error", "CANopenSensor::request: No write function avalable for %s : %s"
            , m_slave->uid(), m_uid); 
            return;
        }
        m_function.writeCB(this, dataJ);

    }
    else if (!strcasecmp (action, "READ")) {
        /*if (!m_function->readCB) goto OnReadError;
        err = (m_function->readCB) (this, &responseJ);
        if (err) goto OnReadError;*/
        //uint val = m_slave->rpdo_mapped[m_register][m_subRegister];
        //printf("DEBUG : writing %d on sensor [%x][%x]", val, m_register, m_subRegister);
        if(!m_function.readCB){
            afb_req_fail_f (request, "read-error", "CANopenSensor::request: No read function avalable for %s : %s"
            , m_slave->uid(), m_uid); 
            return;
        }
        if(m_type == CO_TYPE_SDO){
            afb_req_t current_req = request;
            afb_req_addref(current_req);
            m_slave->Post([this, request]() {
                json_object *responseJ;
                m_function.readCB(this, &responseJ);
                std::cout << "DEBUG : Async read of slave : " << m_slave->id() << " [" << std::hex << m_register << "]:[" << m_subRegister << "] returned " << json_object_get_string(responseJ) << std::endl;
                //*responseJ = json_object_new_int(6584);
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
        err=afb_req_subscribe(request, m_event); 
        if (err){
            afb_req_fail_f (request, "subscribe-error","CANopenSensor::request: fail to subscribe slave=%s sensor=%s"
            , m_slave->uid(), m_uid); 
            return;
        }

    }
    else if (!strcasecmp (action, "UNSUBSCRIBE")) {
        if (m_event) {
            err=afb_req_unsubscribe(request, m_event); 
            if (err) {
                afb_req_fail_f (request, "subscribe-error","CANopenSensor::request: fail to unsubscribe slave=%s sensor=%s"
                , m_slave->uid(), m_uid); 
                return;
            }
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

    /*OnWriteError:
        afb_req_fail_f (request, "write-error", "CANopenSensor::request: fail to write data=%s rtu=%s sensor=%s error=%s"
            , json_object_get_string(dataJ), m_uid, sensor->uid, CANopen_strerror(errno));
        return; 

    OnReadError:
        afb_req_fail_f (request, "read-error", "CANopenSensor::request: fail to read rtu=%s sensor=%s error=%s"
        , m_uid, sensor->uid, CANopen_strerror(errno)); 
        return;

    OnSubscribeError:
        afb_req_fail_f (request, "subscribe-error","CANopenSensor::request: fail to subscribe rtu=%s sensor=%s error=%s"
        , m_uid, sensor->uid, CANopen_strerror(errno)); 
        return;*/
}

int CANopenSensor::eventCreate(){
    if(!m_function.readCB) return ERROR;
    if (!m_event) {
        m_event = afb_api_make_event(m_api, m_uid);
        if (!m_event) {
            AFB_API_ERROR(m_api, "CANopenSensor::eventCreate: fail to create event slave=%s sensor=%s", m_slave->uid(), m_uid);  
            return ERROR;
        }
    }
    m_slave->addSensorEvent(m_register, m_subRegister, m_event);
    return 0;
}
    

int CANopenSensor::coSDOwriteUint8(CANopenSensor* sensor, json_object* inputJ){
    int val = json_object_get_int(inputJ);
    sensor->m_slave->AsyncWrite<uint8_t>(sensor->m_register, sensor->m_subRegister, (uint8_t)val);
    return 0;
}

int CANopenSensor::coSDOwriteUint16(CANopenSensor* sensor, json_object* inputJ){
    int val = json_object_get_int(inputJ);
    sensor->m_slave->AsyncWrite<uint16_t>(sensor->m_register, sensor->m_subRegister, (uint16_t)val);
    return 0;
}

int CANopenSensor::coSDOwriteUint32(CANopenSensor* sensor, json_object* inputJ){
    int val = json_object_get_int(inputJ);
    sensor->m_slave->AsyncWrite<uint32_t>(sensor->m_register, sensor->m_subRegister, (uint32_t)val);
    return 0;
}

int CANopenSensor::coSDOreadUint8(CANopenSensor* sensor, json_object** responseJ){
    int val = sensor->m_slave->Wait(sensor->m_slave->AsyncRead<uint8_t>(sensor->m_register, sensor->m_subRegister));
    // std::cout << "DEBUG : CANopenSensor::coSDOreadUint8 : read val = " << val << std::endl;
    AFB_API_DEBUG(sensor->m_api,"CANopenSensor::coSDOreadUint8 : read val = %d", val);
    *responseJ = json_object_new_int(val);
    AFB_API_DEBUG(sensor->m_api,"CANopenSensor::coSDOreadUint8 : read Json val = %s", json_object_get_string(*responseJ));
    return 0;
}

int CANopenSensor::coSDOreadUint16(CANopenSensor* sensor, json_object** responseJ){
    int val = sensor->m_slave->Wait(sensor->m_slave->AsyncRead<uint16_t>(sensor->m_register, sensor->m_subRegister));
    // std::cout << "DEBUG : CANopenSensor::coSDOreadUint16 : read val = " << val << std::endl;
    AFB_API_DEBUG(sensor->m_api,"CANopenSensor::coSDOreadUint16 : read val = %d", val);
    *responseJ = json_object_new_int(val);
    AFB_API_DEBUG(sensor->m_api,"CANopenSensor::coSDOreadUint16 : read Json val = %s", json_object_get_string(*responseJ));
    return 0;
}

int CANopenSensor::coSDOreadUint32(CANopenSensor* sensor, json_object** responseJ){
    int val = sensor->m_slave->Wait(sensor->m_slave->AsyncRead<uint32_t>(sensor->m_register, sensor->m_subRegister));
    // std::cout << "DEBUG : CANopenSensor::coSDOreadUint8 : read val = " << val << std::endl;
    AFB_API_DEBUG(sensor->m_api,"CANopenSensor::coSDOreadUint32 : read val = %d", val);
    *responseJ = json_object_new_int(val);
    AFB_API_DEBUG(sensor->m_api,"CANopenSensor::coSDOreadUint32 : read Json val = %s", json_object_get_string(*responseJ));
    return 0;
}

// const std::map<std::string, CANopenSensor::TypeCB> CANopenSensor::m_avalableReadCBs{
//     {"uint8",  coSDOreadUint8},
//     {"uint16", coSDOreadUint16},
//     {"uint32", coSDOreadUint32}
// };

const std::map<std::string, CANopenSensor::CANopenFunctionCbS> CANopenSensor::m_SDOfunctionCBs{
    {"uint8", {coSDOreadUint8, coSDOwriteUint8}},
    {"uint16",{coSDOreadUint16, coSDOwriteUint16}},
    {"uint32",{coSDOreadUint32, coSDOwriteUint32}},
};

const std::map<std::string, CANopenSensor::CANopenFunctionCbS> CANopenSensor::m_RPDOfunctionCBs{
    // {"uint8", {coPDOreadUint8, nullptr}},
    // {"uint16",{coPDOreadUint16, nullptr}},
    // {"uint32",{coPDOreadUint32, nullptr}},
};

const std::map<std::string, CANopenSensor::CANopenFunctionCbS> CANopenSensor::m_TPDOfunctionCBs{
    // {"uint8", {nullptr, coPDOwriteUint8}},
    // {"uint16",{nullptr, coPDOwriteUint16}},
    // {"uint32",{nullptr, coPDOwriteUint32}},
};

const std::map<std::string, uint> CANopenSensor::m_AvalableTypes{
    {"SDO", CO_TYPE_SDO},
    {"TPDO", CO_TYPE_TPDO},
    {"RPDO", CO_TYPE_RPDO}
};