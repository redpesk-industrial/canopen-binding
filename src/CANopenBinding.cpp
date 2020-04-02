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

#include "AglCANopen.hpp"
#include "CANopenEncoder.hpp"

#include <ctl-config.h>
#include <filescan-utils.h>

#ifndef ERROR
  #define ERROR -1
#endif

static int CANopenConfig(afb_api_t api, CtlSectionT *section, json_object *rtusJ);

// Config Section definition (note: controls section index should match handle
// retrieval in HalConfigExec)
static CtlSectionT ctrlSections[] = {
    { .key = "plugins", .uid = nullptr, .info = nullptr, .loadCB = PluginConfig, .handle= (void*) &CANopenEncoder::instance(), .actions = nullptr},
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

static void bindingInfo(afb_req_t request){
    AglCANopen* CANopenMaster = (AglCANopen *)afb_req_get_vcbdata(request);
    afb_req_success_f(request, CANopenMaster->infoJ(), NULL);
}

// Static verb not depending on CANopen json config file
static afb_verb_t CtrlApiVerbs[] = {
    { .verb = "ping", .callback = PingTest, .auth = nullptr, .info = "CANopen API ping test", .vcbdata = nullptr, .session = 0, .glob = 0},
    { .verb = "info", .callback = bindingInfo, .auth = nullptr, .info = "display info about the binding", .vcbdata = nullptr, .session = 0, .glob = 0},
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

    // Load CANopen network configuration and start
    CANopenMaster = new AglCANopen(api, rtusJ);
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

    const char* prefix = "canopen";
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
