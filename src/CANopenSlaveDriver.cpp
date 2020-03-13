#include <strings.h>

// if 2 before 1 => conflict with 'is_error' betwin a lely function and a json define named identically
#include "CANopenSlaveDriver.hpp" /*1*/
#include "CANopenSensor.hpp" /*2*/
#include "AglCANopen.hpp"

static void slaveDynRequest(afb_req_t request){
    json_object * queryJ = afb_req_json(request);
    CANopenSlaveDriver * slave = (CANopenSlaveDriver *) afb_req_get_vcbdata(request);
    //slave->Post(slave->request(request, queryJ));
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
    json_object *sensorsJ = NULL;
    char* adminCmd;
    assert (slaveJ);

    err = wrap_json_unpack(slaveJ, "{ss,s?s,ss,s?s,so}",
            "uid", &m_uid,
            "info", &m_info,
            "prefix", &m_prefix,
            "dcf", &m_dcf,
            //"nodId", &m_nodId,
            "sensors", &sensorsJ);
    if (err) {
        AFB_API_ERROR(api, "Fail to parse slave JSON : (%s)", json_object_to_json_string(slaveJ));
        return;
    }

    //Add verd for SDO, MNT, Guard and emergency message / communication 
    //err = afb_api_add_verb(api, m_prefix, m_info, slaveDynRequest, this, nullptr, 0, 0);

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
    } else std::cout << "DEBUG : verb \"" << adminCmd << "\" created !" << "\n";

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
    int idx;
    uint16_t regId;
    int subidx;
    uint8_t subRegId;
    int val;
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
        err= wrap_json_unpack(dataJ, "{si si si si!}",
            "id", &idx,
            "subid", &subidx,
            "val", &val,
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

        regId = uint16_t(idx);
        subRegId = uint8_t(subidx);
        //printf("DEBUG : SDO write to %s[%x][%x] val %x\n with %d bytes", m_prefix, idx, subidx, val, size);
        switch (size)
        {
        case 1:
            AsyncWrite<uint8_t>(regId, subRegId, (uint8_t)val);
            break;
        case 2:
            AsyncWrite<uint16_t>(regId, subRegId, (uint16_t)val);
            break;
        case 3:
            AsyncWrite<uint32_t>(regId, subRegId, val);
            break;
        case 4:
            AsyncWrite<uint32_t>(regId, subRegId, val);
            break;
        // case 5:
        //     AsyncWrite<std::string>(regId, subRegId, "");
        //     break;
        default:
            afb_req_fail_f(
                request,
                "query-error",
                "CANopenSlaveDriver::request: invalid size %d. Avalable size (in byte) are 1, 2, 3 or 4",
                size
            );
            break;
        }

    } else if (!strcasecmp (action, "READ")) {
        err= wrap_json_unpack(dataJ, "{si si !}",
            "id", &idx,
            "subid", &subidx
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

        regId = uint16_t(idx);
        subRegId = uint8_t(subidx);

        afb_req_t current_req = request;
        afb_req_addref(current_req);

        Post([this, request, regId, subRegId]() {
            auto v = Wait(AsyncRead<uint32_t>(regId, subRegId));
            std::cout << "DEBUG : Async read of slave " << (int)id() << " [" << std::hex << regId << "]:[" << subRegId << "] returned " << v << std::endl;
            afb_req_success(request, json_object_new_int(v), NULL);
            afb_req_unref(request);
        });
        return;

     } //else if (!strcasecmp (action, "SUBSCRIBE")) {
        
    //     /*try {
    //         AglCANopen::avalableTypeCBs.at("UINT32")(1,0);
    //     } catch(const std::out_of_range &e) {
    //         std::cerr << "Exception at " << e.what() << std::endl;
    //     }*/
    //     // err= this->eventCreate (&responseJ);
    //     // //if (err) goto OnSubscribeError;
    //     // err=afb_req_subscribe(request, m_event); 
    //     // //if (err) goto OnSubscribeError;

    // }  else if (!strcasecmp (action, "UNSUBSCRIBE")) {   // Fulup ***** Virer l'event quand le count est à zero
    //     if (m_event) {
    //         err=afb_req_unsubscribe(request, m_event); 
    //         if (err) goto OnSubscribeError;
    //     }
    // }
    else {
        afb_req_fail_f (request, "syntax-error", "CANopenSensor::request: action='%s' UNKNOWN rtu=%s query=%s"
            , action, m_uid, json_object_get_string(queryJ));
        return; 
    }
    // everything looks good let's response
    afb_req_success(request, responseJ, NULL);
    return;
}

void CANopenSlaveDriver::addSensorEvent(uint16_t reg, uint8_t subreg, afb_event_t event){
    Post([this, reg, subreg, event]() {
        m_sensorEventQueue.insert(m_sensorEventQueue.end(),{reg, subreg, event});
    });
}