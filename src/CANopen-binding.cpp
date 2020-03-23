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

#include "AglCANopen.hpp"
#include "CANopen-encoder.hpp"
//#include "CANopen-driver.hpp" /*1*/
//#include "CANopen-binding.hpp" /*2*/

#include <ctl-config.h>
#include <filescan-utils.h>

#include <iostream> //temp
//#include <map>

#ifndef ERROR
  #define ERROR -1
#endif

static int CANopenConfig(afb_api_t api, CtlSectionT *section, json_object *rtusJ);

// Config Section definition (note: controls section index should match handle
// retrieval in HalConfigExec)
static CtlSectionT ctrlSections[] = {
    { .key = "plugins", .uid = nullptr, .info = nullptr, .loadCB = PluginConfig, .handle= nullptr, .actions = nullptr},
    //{ .key = "onload", .uid = nullptr, .info = nullptr, .loadCB = OnloadConfig, .handle = nullptr, .actions = nullptr },
    { .key = "canopen", .uid = nullptr, .info = nullptr, .loadCB = CANopenConfig, .handle = nullptr, .actions = nullptr },
    { .key = nullptr, .uid = nullptr, .info = nullptr, .loadCB = nullptr,  .handle = nullptr, .actions = nullptr }
};

static void PingTest (afb_req_t request) {
    static int count=0;
    char response[32];
    json_object *queryJ = afb_req_json(request);

    snprintf (response, sizeof(response), "Pong=%d", count++);
    AFB_API_NOTICE (request->api, "CANopen:ping count=%d query=%s", count, json_object_get_string(queryJ));
    afb_req_success_f(request,json_object_new_string(response), NULL);
}

// static void InfoRtu (afb_req_t request) {
//     json_object *elemJ;
//     int err, idx;
//     int verbose=0;
//     int length =0;
//     CANopenRtuT *rtus = (CANopenRtuT*) afb_req_get_vcbdata(request);
//     json_object *queryJ = afb_req_json(request);
//     json_object *responseJ = json_object_new_array();

//     err = wrap_json_unpack(queryJ, "{s?i s?i !}", "verbose", &verbose, "length", &length);
//     if (err) {
//         afb_req_fail (request, "ListRtu: invalid 'json' query=%s", json_object_get_string(queryJ));
//         return;
//     }

//     // loop on every defined RTU
//     for (idx=0; rtus[idx].uid; idx++) {
//         switch (verbose) {
//             case 0:
//                 wrap_json_pack (&elemJ, "{ss ss}", "uid", rtus[idx].uid);
//                 break;
//             case 1:
//             default:
//                 wrap_json_pack (&elemJ, "{ss ss ss}", "uid", rtus[idx].uid, "uri",rtus[idx].uri, "info", rtus[idx].info);
//                 break;
//             case 2:
//                 err= CANopenRtuIsConnected (request->api, &rtus[idx]);
//                 if (err <0) {
//                     wrap_json_pack (&elemJ, "{ss ss ss}", "uid", rtus[idx].uid, "uri",rtus[idx].uri, "info", rtus[idx].info);;
//                 } else {
//                     wrap_json_pack (&elemJ, "{ss ss ss sb}", "uid", rtus[idx].uid, "uri",rtus[idx].uri, "info", rtus[idx].info, "connected", err);;
//                 }
//                 break;
//         }
//         json_object_array_add(responseJ, elemJ);
//     }
//     afb_req_success(request, responseJ, NULL);
// }

// Static verb not depending on CANopen json config file
static afb_verb_t CtrlApiVerbs[] = {
    /* VERB'S NAME         FUNCTION TO CALL         SHORT DESCRIPTION */
    { .verb = "ping", .callback = PingTest, .auth = nullptr, .info = "CANopen API ping test", .vcbdata = nullptr, .session = 0, .glob = 0},
    // verb "info" not handle for now
    // { .verb = "info", .callback = InfoRtu, .auth = nullptr, .info = "CANopen List RTUs", .vcbdata = nullptr, .session = 0, .glob = 0},
    { .verb = nullptr, .callback = nullptr, .auth = nullptr, .info = nullptr, .vcbdata = nullptr, .session = 0, .glob = 0} /* marker for end of the array */
};

static int CtrlLoadStaticVerbs (afb_api_t api, afb_verb_t *verbs, void *vcbdata) {
    int errcount=0;

    for (int idx=0; verbs[idx].verb; idx++) {
        errcount+= afb_api_add_verb(api, CtrlApiVerbs[idx].verb, CtrlApiVerbs[idx].info, CtrlApiVerbs[idx].callback, vcbdata, 0, 0,0);
    }

    return errcount;
};


static int CANopenConfig(afb_api_t api, CtlSectionT *section, json_object *rtusJ) {
    int err;
    AglCANopen *CANopenMaster;

    // everything is done during initial config call
    if (!rtusJ) return 0;

    // Canopen can only have one Master;
    if (json_object_is_type(rtusJ, json_type_array)) {
        AFB_API_ERROR(api, "CANopenConfig : Multiple CANopen forbiden");
        return ERROR;
    }

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

    ctrlConfig->external = (void*)new CANopenEncoder();

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
