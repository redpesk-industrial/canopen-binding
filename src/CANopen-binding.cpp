/*
* Copyright (C) 2016-2019 "IoT.bzh"
* Author Fulup Ar Foll <fulup@iot.bzh>
* Author Fulup Ar Foll <romain@iot.bzh>
* Author Fulup Ar Foll <sebastien@iot.bzh>
*
* Licensed under the Apache License, Version 2.0 (the "License");
* you may not use this file except in compliance with the License.
* You may obtain a copy of the License at
*
*   http://www.apache.org/licenses/LICENSE-2.0
*
* Unless required by applicable law or agreed to in writing, software
* distributed under the License is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
* See the License for the specific language governing permissions and
* limitations under the License.
*/


// Le contexte de sensor loader au moment de l'API n'est retrouvé avec le request ****

#include "CANopen-driver.hpp" /*1*/
#include "CANopen-binding.hpp" /*2*/

#include <ctl-config.h>
#include <filescan-utils.h>

#include <iostream> //temp
#include <map>

#define ERROR -1

static int CANopenConfig(afb_api_t api, CtlSectionT *section, json_object *rtusJ);

static char* fullPathToDCF(afb_api_t api, const char *dcfFile){
    
    int err = 0;

    char *fullpath = nullptr;
    char *filename = nullptr;

    //std::cout << "search for DCF file..." << std::endl;

    json_object * pathToDCF = ScanForConfig (CONTROL_CONFIG_PATH, CTL_SCAN_RECURSIVE, dcfFile, "");
    
    if(!pathToDCF){
        AFB_API_ERROR(api, "CANopenLoadOne: fail to find dcf file '%s'", dcfFile);
        return nullptr;
    }
    err = wrap_json_unpack(pathToDCF, "[{ss, ss}]",
                            "fullpath", &fullpath,
                            "filename", &filename);
    if (err) {
        AFB_API_ERROR(api, "Fail to parse pathToDCF JSON : (%s)", json_object_to_json_string(pathToDCF));
        return nullptr;
    }
    //std::cout << "pathToDCF = " << json_object_to_json_string(pathToDCF) << std::endl;

    strncat(fullpath, "/" , strlen(fullpath));
    strncat(fullpath, dcfFile , strlen(fullpath));

    return fullpath;
}

// Config Section definition (note: controls section index should match handle
// retrieval in HalConfigExec)
static CtlSectionT ctrlSections[] = {
    //{ .key = "plugins", .loadCB = PluginConfig, .handle= mbEncoderRegister},
    //{ .key = "onload", .uid = nullptr, .info = nullptr, .loadCB = OnloadConfig, .handle = nullptr, .actions = nullptr },
    { .key = "canopen", .uid = nullptr, .info = nullptr, .loadCB = CANopenConfig, .handle = nullptr, .actions = nullptr },
    { .key = nullptr, .uid = nullptr, .info = nullptr, .loadCB = nullptr,  .handle = nullptr, .actions = nullptr }
};

static void PingTest (afb_req_t request) {
    static int count=0;
    char response[32];
    json_object *queryJ =  afb_req_json(request);

    snprintf (response, sizeof(response), "Pong=%d", count++);
    AFB_API_NOTICE (request->api, "CANopen:ping count=%d query=%s", count, json_object_get_string(queryJ));
    afb_req_success_f(request,json_object_new_string(response), NULL);
}

static void InfoRtu (afb_req_t request) {
    json_object *elemJ;
    int err, idx;
    int verbose=0;
    int length =0;
    CANopenRtuT *rtus = (CANopenRtuT*) afb_req_get_vcbdata(request);
    json_object *queryJ =  afb_req_json(request);
    json_object *responseJ= json_object_new_array();

    err= wrap_json_unpack(queryJ, "{s?i s?i !}", "verbose", &verbose, "length", &length);
    if (err) {
        afb_req_fail (request, "ListRtu: invalid 'json' query=%s", json_object_get_string(queryJ));
        return;
    }

    // loop on every defined RTU
    for (idx=0; rtus[idx].uid; idx++) {
        switch (verbose) {
            case 0:
                wrap_json_pack (&elemJ, "{ss ss}", "uid", rtus[idx].uid);
                break;
            case 1:
            default:
                wrap_json_pack (&elemJ, "{ss ss ss}", "uid", rtus[idx].uid, "uri",rtus[idx].uri, "info", rtus[idx].info);
                break;
            case 2:
                err= CANopenRtuIsConnected (request->api, &rtus[idx]);
                if (err <0) {
                    wrap_json_pack (&elemJ, "{ss ss ss}", "uid", rtus[idx].uid, "uri",rtus[idx].uri, "info", rtus[idx].info);;
                } else {
                    wrap_json_pack (&elemJ, "{ss ss ss sb}", "uid", rtus[idx].uid, "uri",rtus[idx].uri, "info", rtus[idx].info, "connected", err);;
                }
                break;
        }

        json_object_array_add(responseJ, elemJ);
    }
    afb_req_success(request, responseJ, NULL);
}

// Static verb not depending on CANopen json config file
static afb_verb_t CtrlApiVerbs[] = {
    /* VERB'S NAME         FUNCTION TO CALL         SHORT DESCRIPTION */
    { .verb = "ping", .callback = PingTest, .auth = nullptr, .info = "CANopen API ping test", .vcbdata = nullptr, .session = 0, .glob = 0},
    { .verb = "info", .callback = InfoRtu, .auth = nullptr, .info = "CANopen List RTUs", .vcbdata = nullptr, .session = 0, .glob = 0},
    { .verb = NULL, .callback = nullptr, .auth = nullptr, .info = nullptr, .vcbdata = nullptr, .session = 0, .glob = 0} /* marker for end of the array */
};

static int CtrlLoadStaticVerbs (afb_api_t api, afb_verb_t *verbs, void *vcbdata) {
    int errcount=0;

    for (int idx=0; verbs[idx].verb; idx++) {
        errcount+= afb_api_add_verb(api, CtrlApiVerbs[idx].verb, CtrlApiVerbs[idx].info, CtrlApiVerbs[idx].callback, vcbdata, 0, 0,0);
    }

    return errcount;
};
/*static void RtuDynRequest(afb_req_t request) {

    // retrieve action handle from request and execute the request
    json_object *queryJ = afb_req_json(request);
    CANopenRtuT* rtu= (CANopenRtuT*) afb_req_get_vcbdata(request);
    CANopenRtuRequest (request, rtu, queryJ);
}*/

static void SensorDynRequest(afb_req_t request) {

    // retrieve action handle from request and execute the request
    json_object *queryJ = afb_req_json(request);
    CANopenSensorT* sensor = (CANopenSensorT*) afb_req_get_vcbdata(request);
    CANopenSensorRequest (request, sensor, queryJ);
}

static int SensorLoadOne(afb_api_t api, CANopenSlaveT *slave, CANopenSensorT *sensor, json_object *sensorJ) {
    int err = 0;
    const char *type=NULL;
    const char *format=NULL;
    const char *privilege=NULL;
    afb_auth_t *authent=NULL;
    json_object *argsJ=NULL;
    char* apiverb;
    //CANopenSourceT source;

    // should already be allocated
    assert (sensorJ);

    // set default values
    memset(sensor, 0, sizeof (CANopenSensorT));
    sensor->slave = slave;
    //sensor->iddle = rtu->iddle;
    sensor->count = 1;

    err = wrap_json_unpack(sensorJ, "{ss,ss,si,s?s,s?s,s?s,s?i,s?i,s?i,s?o !}",
                "uid", &sensor->uid,
                "type", &type,
                "register", &sensor->registry,
                "info", &sensor->info,
                "privilege", &privilege,
                "format", &format,
                "iddle", &sensor->iddle,
                "count", &sensor->count,
                "args", &argsJ);
    if (err) {
        AFB_API_ERROR(api, "SensorLoadOne: Fail to parse sensor: %s", json_object_to_json_string(sensorJ));
        return ERROR;
    }

    err = asprintf (&apiverb, "%s/%s", slave->prefix, sensor->uid);
    err = afb_api_add_verb(api, (const char*) apiverb, sensor->info, SensorDynRequest, sensor, authent, 0, 0);
    if (err) {
        AFB_API_ERROR(api, "SensorLoadOne: fail to register API verb=%s", apiverb);
        return ERROR;
    }

    return 0;  
}

static int SlaveLoadOne(afb_api_t api, CANopenRtuT *rtu, CANopenSlaveT *slave, json_object *slaveJ) {
    int err = 0;
    json_object *sensorsJ = NULL;
    CtlConfigT *ctrlConfig = (CtlConfigT*)afb_api_get_userdata(api);
    AglCANopen *CanMaster = (AglCANopen*)ctrlConfig->external;

    // should already be allocated
    assert (slaveJ);

    //memset(rtu, 0, sizeof (CANopenRtuT)); // default is empty

    // set default values
    memset(slave, 0, sizeof (CANopenSlaveT));
    slave->rtu   = rtu;
    slave->count = 1;
    err = wrap_json_unpack(slaveJ, "{ss,s?s,ss,s?s,si,so !}",
            "uid", &slave->uid,
            "info", &slave->info,
            "prefix", &slave->prefix,
            "dcf", &slave->dcf,
            "nodId", &slave->nodId,
            "sensors", &sensorsJ);
    if (err) {
        AFB_API_ERROR(api, "Fail to parse rtu JSON : (%s)", json_object_to_json_string(slaveJ));
        return ERROR;
    }
    // loop on sensors
    if (json_object_is_type(sensorsJ, json_type_array)) {
        int count = (int)json_object_array_length(sensorsJ);
        slave->sensors= (CANopenSensorT*)calloc(count + 1, sizeof (CANopenSensorT));

        for (int idx = 0; idx < count; idx++) {
            json_object *sensorJ = json_object_array_get_idx(sensorsJ, idx);
            err = SensorLoadOne(api, slave, &slave->sensors[idx], sensorJ);
            if (err) return ERROR;
        }

    } else {
        slave->sensors= (CANopenSensorT*) calloc(2, sizeof(CANopenSensorT));
        err= SensorLoadOne(api, slave, &slave->sensors[0], sensorsJ);
        if (err) return ERROR;
    }
    /*//Atache a slave to the master
    if(!strlen(slave->dcf)){
        const char *dcfFile = fullPathToDCF(api, slave->dcf);
        if (!strlen(dcfFile)) {
            AFB_API_ERROR(api, "CANopenLoadOne: fail to find =%s", slave->dcf);
            return ERROR;
        }
        CanMaster.addSlave(slave->nodId, dcfFile);
    }*/
    CanMaster->addSlave(slave->nodId);
    return 0;
}

static int CANopenLoadOne(afb_api_t api, CANopenRtuT *rtu, json_object *rtuJ) {
    int err = 0;
    //json_object *sensorsJ = NULL;
    json_object *slavesJ = NULL;
    CtlConfigT *ctrlConfig = nullptr;
    AglCANopen *CanMaster = nullptr;
    // should already be allocated
    assert (rtuJ); 
    assert (api);

    memset(rtu, 0, sizeof (CANopenRtuT)); // default is empty
    rtu->nodId = 1;
    err = wrap_json_unpack(rtuJ, "{ss,s?s,ss,s?s,s?i,so !}",
            "uid", &rtu->uid,
            "info", &rtu->info,
            "uri", &rtu->uri,
            "dcf", &rtu->dcf,
            "nodId", &rtu->nodId,
            "slaves", &slavesJ);
    if (err) {
        AFB_API_ERROR(api, "Fail to parse rtu JSON : (%s)", json_object_to_json_string(rtuJ));
        return ERROR;
    }
    const char *dcfFile = fullPathToDCF(api, rtu->dcf);
    if (!strlen(dcfFile)) {
        AFB_API_ERROR(api, "CANopenLoadOne: fail to find =%s", rtu->dcf);
        return ERROR;
    }
    //std::cout << "DCF file found : " << dcfFile << std::endl;
    // if uri is provided let's try to connect now
    
    if (rtu->uri) {
        //err = CANopenRtuConnect (api, rtu);
        ctrlConfig = (CtlConfigT*)afb_api_get_userdata(api);
        CanMaster = new AglCANopen(rtu->uri, dcfFile, rtu->nodId);
        ctrlConfig->external = CanMaster;
        if (!CanMaster->chanIsOpen()) {
            AFB_API_ERROR(api, "CANopenLoadOne: fail to connect can uid=%s uri=%s", rtu->uid, rtu->uid);
            return ERROR;
        }
    }

    // loop on slaves
    if (json_object_is_type(slavesJ, json_type_array)) {
        int count = (int)json_object_array_length(slavesJ);
        rtu->slaves= (CANopenSlaveT*)calloc(count + 1, sizeof (CANopenSlaveT));

        for (int idx = 0; idx < count; idx++) {
            json_object *slaveJ = json_object_array_get_idx(slavesJ, idx);
            err = SlaveLoadOne(api, rtu, &rtu->slaves[idx], slaveJ);
            if (err) return ERROR;
        }
    } else {
        rtu->slaves= (CANopenSlaveT*) calloc(2, sizeof(CANopenSlaveT));
        err= SlaveLoadOne(api, rtu, &rtu->slaves[0], slavesJ);
        if (err) return ERROR;
    }

    //CanMaster->start();
    CanMaster->start(afb_daemon_get_event_loop());
    /*struct sd_event_source* event_source = nullptr;

    auto handler = [](sd_event_source*, int, uint32_t, void* userdata) {
        lely::ev::Poll poll(static_cast<ev_poll_t*>(userdata));
        poll.wait(0);
        return 0;
    };
    auto userdata = const_cast<void*>(static_cast<const void*>(static_cast<ev_poll_t*>(CanMaster-> poll.get_poll())));
    sd_event_add_io(afb_daemon_get_event_loop(), &event_source, poll.get_fd(), EPOLLIN, handler, userdata);
    */
    return 0;   
}

/*static int CANopenConfig(afb_api_t api, CtlSectionT *section, json_object *rtusJ) {
    CANopenRtuT *rtus;
    int err;

    // everything is done during initial config call
    if (!rtusJ) return 0;

    // canopen array is close with a nullvalue;
    if (json_object_is_type(rtusJ, json_type_array)) {
        int count = (int)json_object_array_length(rtusJ);
        rtus =  (CANopenRtuT*) calloc(count + 1, sizeof (CANopenRtuT));

        for (int idx = 0; idx < count; idx++) {
            json_object *rtuJ = json_object_array_get_idx(rtusJ, idx);
            err = CANopenLoadOne(api, &rtus[idx], rtuJ);
            if (err) return ERROR;
        }

    } else {
        rtus = (CANopenRtuT*)calloc(2, sizeof (CANopenRtuT));
        err = CANopenLoadOne(api, &rtus[0], rtusJ);
        if (err) return ERROR;
    }


    // add static controls verbs
    err = CtrlLoadStaticVerbs (api, CtrlApiVerbs, (void*) rtus);
    if (err) {
        AFB_API_ERROR(api, "CtrlLoadOneApi fail to Registry static API verbs");
        return ERROR;
    }
    
    return 0;
    AFB_API_ERROR (api, "Fail to initialise CANopen check Json Config");
    return -1;    
}*/

static int CANopenConfig(afb_api_t api, CtlSectionT *section, json_object *rtusJ) {
    //CANopenRtuT *rtus;
    int err;
    //std::map<int,std::shared_ptr<AglCANopen>> *CANopenMasters;
    AglCANopen *CANopenMaster;

    // everything is done during initial config call
    if (!rtusJ) return 0;

    // Canopen can only have one Master;
    if (json_object_is_type(rtusJ, json_type_array)) {
        AFB_API_ERROR(api, "CANopenConfig : Multiple CANopen forbiden");
        return ERROR;
    }

    //std::cout << "DEBEUG INFO : rtuJ = " << json_object_to_json_string(rtusJ) << "\n";

    CANopenMaster = new AglCANopen(api, rtusJ, afb_daemon_get_event_loop());
    if (!CANopenMaster->isRuning()) return ERROR;


    // add static controls verbs
    err = CtrlLoadStaticVerbs (api, CtrlApiVerbs, (void*) CANopenMaster);
    if (err) {
        AFB_API_ERROR(api, "CtrlLoadOneApi fail to Registry static API verbs");
        return ERROR;
    }
    
    return 0;
    AFB_API_ERROR (api, "Fail to initialise CANopen check Json Config");
    return -1;    
}


static int CtrlInitOneApi(afb_api_t api) {
    int err = 0;

    // retrieve section config from api handle
    CtlConfigT* ctrlConfig = (CtlConfigT*)afb_api_get_userdata(api);

    err = CtlConfigExec(api, ctrlConfig);
    if (err) {
        AFB_API_ERROR(api, "Error at CtlConfigExec step");
        return err;
    }

    return err;
}

//Fonction de pré init de l'API
static int CtrlLoadOneApi(void* vcbdata, afb_api_t api) {
    CtlConfigT* ctrlConfig = (CtlConfigT*)vcbdata;

    // save closure as api's data context
    afb_api_set_userdata(api, ctrlConfig);

    // load section for corresponding API
    int error = CtlLoadSections(api, ctrlConfig, ctrlSections);

    // init and seal API function
    afb_api_on_init(api, CtrlInitOneApi);
    afb_api_seal(api);

    return error;    
}

int afbBindingEntry(afb_api_t api) {
    int status = 0;
    char *searchPath = nullptr;
    const char *envConfig = nullptr;
    afb_api_t handle;

    AFB_API_NOTICE(api, "Controller in afbBindingEntry");

    envConfig= getenv("CONTROL_CONFIG_PATH");
    if (!envConfig) envConfig = CONTROL_CONFIG_PATH;

    status=asprintf (&searchPath,"%s:%s/etc", envConfig, GetBindingDirPath(api));
    AFB_API_NOTICE(api, "Json config directory : %s", searchPath);

    const char* prefix = "control";
    const char* configPath = CtlConfigSearch(api, searchPath, prefix);
    if (!configPath) {
        AFB_API_ERROR(api, "afbBindingEntry: No %s-%s* config found in %s ", prefix, GetBinderName(), searchPath);
        status = ERROR;
        free(searchPath);
        return status;
    }

    // load config file and create API
    CtlConfigT* ctrlConfig = CtlLoadMetaData(api, configPath);
    if (!ctrlConfig) {
        AFB_API_ERROR(api, "afbBindingEntry No valid control config file in:\n-- %s", configPath);
        status = ERROR;
        free(searchPath);
        return status;
    }

    AFB_API_NOTICE(api, "Controller API='%s' info='%s'", ctrlConfig->api, ctrlConfig->info);

    // create one API per config file (Pre-V3 return code ToBeChanged)
    handle = afb_api_new_api(api, ctrlConfig->api, ctrlConfig->info, 1, CtrlLoadOneApi, ctrlConfig);
    status = (handle) ? 0 : -1;

    return status;
}
