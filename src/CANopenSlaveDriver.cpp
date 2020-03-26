#include <string.h>

// if 2 before 1 => conflict with 'is_error' betwin a lely function and a json define named identically
#include "CANopenSlaveDriver.hpp" /*1*/
#include "CANopenSensor.hpp" /*2*/
#include "AglCANopen.hpp"
#include "CANopenGlue.hpp"

static void slaveDynRequest(afb_req_t request){
    json_object * queryJ = afb_req_json(request);
    CANopenSlaveDriver * slave = (CANopenSlaveDriver *) afb_req_get_vcbdata(request);
    slave->request(request, queryJ);
}

CANopenSlaveDriver::CANopenSlaveDriver(
        ev_exec_t * exec,
        lely::canopen::BasicMaster& master,
        afb_api_t api,
        json_object * slaveJ,
        uint8_t nodId
    ) : lely::canopen::FiberDriver(exec, master, nodId)
{
    int err = 0;
    json_object *sensorsJ = nullptr;
    char* adminCmd;
    assert (slaveJ);

    err = wrap_json_unpack(slaveJ, "{ss,s?s,ss,s?s,s?o,so}",
            "uid", &m_uid,
            "info", &m_info,
            "prefix", &m_prefix,
            "dcf", &m_dcf,
            "onconf", &m_onconfJ,
            "sensors", &sensorsJ);
    if (err) {
        AFB_API_ERROR(api, "Fail to parse slave JSON : (%s)", json_object_to_json_string(slaveJ));
        return;
    }

    // if not API prefix let's use RTU uid
    if (!m_prefix) m_prefix= m_uid;

    // create an admin command for SDO row communication on the CANopen network
    afb_auth_t * authent = (afb_auth_t*) calloc(1, sizeof (afb_auth_t));
    authent->type = afb_auth_Permission;
    authent->text = "superadmin";

    err=asprintf (&adminCmd, "%s/%s", m_prefix, "superadmin");
    err= afb_api_add_verb(api, adminCmd, m_info, slaveDynRequest, this, authent, 0, 0);
    if (err) {
        AFB_API_ERROR(api, "CANopenSlaveDriver: fail to register API uid=%s verb=%s info=%s", m_uid, adminCmd, m_info);
        return;
    }

    if (err) {
        AFB_API_ERROR(api, "CANopenSlaveDriver: fail to register API verb=%s", m_prefix);
        return;
    }

    // loop on sensors
    if (json_object_is_type(sensorsJ, json_type_array)) {
        int count = (int)json_object_array_length(sensorsJ);
        m_sensors = std::vector<std::shared_ptr<CANopenSensor>>((size_t)count);
        for (int idx = 0; idx < count; idx++) {
            json_object *sensorJ = json_object_array_get_idx(sensorsJ, idx);
            m_sensors[idx] = std::make_shared<CANopenSensor>(api, sensorJ, this);
        }

    } else {
        m_sensors[0] = std::make_shared<CANopenSensor>(api, sensorsJ, this);
    }
    return;
}

void CANopenSlaveDriver::request (afb_req_t request,  json_object * queryJ) {

    const char *action;
    json_object *dataJ = nullptr;
    json_object *responseJ = nullptr;
    json_object * regJ;
    json_object * valJ;
    int reg;
    double val;
    int size;
    int err;

    err= wrap_json_unpack(queryJ, "{ss s?o !}",
        "action", &action,
        "data", &dataJ
    );

    if (err) {
        afb_req_fail_f(
            request,
            "query-error",
            "CANopenSlaveDriver::request: invalid 'json' rtu=%s query=%s",
            m_prefix, json_object_get_string(queryJ)
        );
        return;
    }

    if (!strcasecmp(action, "WRITE")) {
        err= wrap_json_unpack(dataJ, "{so so si!}",
            "reg", &regJ,
            "val", &valJ,
            "size", &size
        );

        if (err) {
            afb_req_fail_f(
                request,
                "query-error",
                "CANopenSlaveDriver::request: invalid %s action data 'json' rtu=%s data=%s",
                action, m_uid, json_object_get_string(dataJ)
            );
            return;
        }

        try{
            reg = get_data_int(regJ);
        }catch(std::runtime_error& e){
            afb_req_fail_f(
                request,
                "query-error",
                "CANopenSlaveDriver::request: %s => %s register convertion error \nwhat() : %s",
                m_uid, action, e.what()
            );
            return;
        }
        uint16_t idx = ((uint32_t)reg & 0x00ffff00)>>8;
        uint8_t subIdx = (uint32_t)reg & 0x000000ff;

        try{
            val = get_data_double(valJ);
        }catch(std::runtime_error& e){
            afb_req_fail_f(
                request,
                "query-error",
                "CANopenSlaveDriver::request: %s => %s val convertion error \nwhat() : %s",
                m_uid, action, e.what()
            );
            return;
        }

        AFB_REQ_DEBUG(request, "send value 0x%x at register[0x%x][0x%x] of slave '%s'", (uint32_t)val, idx, subIdx, m_uid);
        switch (size)
        {
        case 1:
            AsyncWrite<uint8_t>(idx, subIdx, (uint8_t)val);
            break;
        case 2:
            AsyncWrite<uint16_t>(idx, subIdx, (uint16_t)val);
            break;
        case 3:
            AsyncWrite<uint32_t>(idx, subIdx, (uint32_t)val);
            break;
        case 4:
            AsyncWrite<uint32_t>(idx, subIdx, (uint32_t)val);
            break;
        default:
            afb_req_fail_f(
                request,
                "query-error",
                "CANopenSlaveDriver::request: invalid size %d. Available size (in byte) are 1, 2, 3 or 4",
                size
            );
            break;
        }

    } else if (!strcasecmp (action, "READ")) {
        err= wrap_json_unpack(dataJ, "{so !}",
            "reg", &regJ
        );
        
        if (err) {
            afb_req_fail_f(
                request,
                "query-error",
                "CANopenSensor::request: invalid %s action data 'json' rtu=%s data=%s",
                action, m_uid, json_object_get_string(dataJ)
            );
            return;
        }

        try{
            reg = get_data_int(regJ);
        }catch(std::runtime_error& e){
            afb_req_fail_f(
                request,
                "query-error",
                "CANopenSlaveDriver::request: %s => %s register convertion error \nwhat() : %s",
                m_uid, action, e.what()
            );
            return;
        }
        uint16_t idx = ((uint32_t)reg & 0x00ffff00)>>8;
        uint8_t subIdx = (uint32_t)reg & 0x000000ff;

        afb_req_t current_req = request;
        afb_req_addref(current_req);

        // Use "Post" to avoid asynchronous conflicts
        Post([this, request, idx, subIdx]() {
            auto v = Wait(AsyncRead<uint32_t>(idx, subIdx));
            AFB_REQ_DEBUG(request, "DEBUG : Async read of slave %s [0x%x]:[0x%x] returned 0x%x", m_uid, idx, subIdx, v);
            afb_req_success(request, json_object_new_int64(v), NULL);
            afb_req_unref(request);
        });
        return;

    }
    else {
        afb_req_fail_f (request, "syntax-error", "CANopenSensor::request: action='%s' UNKNOWN rtu=%s query=%s"
            , action, m_uid, json_object_get_string(queryJ));
        return;
    }
    // everything looks good let's response
    afb_req_success(request, responseJ, NULL);
    return;
}

void CANopenSlaveDriver::addSensorEvent(CANopenSensor * sensor){ 
    
    // Use "Post" to avoid asynchronous conflicts
    Post([this, sensor]() {
        m_sensorEventQueue.insert(m_sensorEventQueue.end(), sensor);
    });
}

void CANopenSlaveDriver::delSensorEvent(CANopenSensor* sensor){
    
    // Use "Post" to avoid asynchronous conflicts
    Post([this, sensor]() {
        for(auto q = m_sensorEventQueue.begin() ; q != m_sensorEventQueue.end();){
            if(!strcasecmp((*q)->uid(), sensor->uid())){
                q = m_sensorEventQueue.erase(q);
            }
            else q++;
        }
    });
}

void CANopenSlaveDriver::slavePerStartConfig(json_object * conf){
    const char * actionInfo;
    int reg;
    int size;
    double data;
    int err;
    json_object * dataJ;
    json_object * regJ;

    err = wrap_json_unpack(conf, "{s?s,so,si,so}",
        "info", &actionInfo,
        "register", &regJ,
        "size", &size,
        "data", &dataJ);
    if (err) {
        AFB_ERROR("%s->slavePerStartConfig : Fail to parse slave JSON : (%s)", m_uid, json_object_to_json_string(conf));
        return;
    }

    if(strlen(actionInfo))
        AFB_NOTICE("%s->slavePerStartConfig : %s", m_uid, actionInfo);
    else AFB_NOTICE("%s->slavePerStartConfig", m_uid);

    // Get register and sub register from the parsed register
    try{
        reg = get_data_int(regJ);
    }catch(std::runtime_error& e){
        AFB_ERROR("%s->slavePerStartConfig : %s", m_uid, e.what());
        return;
    }
    uint16_t idx = ((uint32_t)reg & 0x00ffff00)>>8;
    uint8_t subIdx = (uint32_t)reg & 0x000000ff;

    try{
        data = get_data_double(dataJ);
    }catch(std::runtime_error& e){
        AFB_ERROR("%s->slavePerStartConfig : %s", m_uid, e.what());
        return;
    }
    try{
        switch (size)
        {
        case 1:
            Wait(AsyncWrite<uint8_t>(idx, subIdx, (uint8_t)data));
            break;
        case 2:
            Wait(AsyncWrite<uint16_t>(idx, subIdx, (uint16_t)data));
            break;
        case 3:
            Wait(AsyncWrite<uint32_t>(idx, subIdx, (uint32_t)data));
            break;
        case 4:
            Wait(AsyncWrite<uint32_t>(idx, subIdx, (uint32_t)data));
            break;
        default:
            AFB_ERROR(
                "%s->slavePerStartConfig : invalid size %d. Available size (in byte) are 1, 2, 3 or 4",
                m_uid,
                size
            );
            return;
        }
    }catch(lely::canopen::SdoError& e){
        AFB_ERROR("%s->slavePerStartConfig : could not configure register [0x%x][0x%x] with value 0X%x \nwhat() : %s", m_uid, idx, subIdx, (uint32_t)data, e.what());
    }
}