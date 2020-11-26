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

#include "CANopenMaster.hpp"
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
    CANopenMaster* CO_Master = (CANopenMaster *)afb_req_get_vcbdata(request);
    afb_req_success_f(request, CO_Master->infoJ(), NULL);
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
    CANopenMaster *CO_Master;

    // everything is done during initial config call
    if (!rtusJ) return 0;

    // Canopen can only have one Master;
    if (json_object_is_type(rtusJ, json_type_array)) {
        AFB_API_ERROR(api, "CANopenConfig : Multiple CANopen forbiden");
        return ERROR;
    }

    // Load CANopen network configuration and start
    CO_Master = new CANopenMaster(api, rtusJ);
    if (!CO_Master->isRuning()) return ERROR;

    // add static controls verbs
    err = CtrlLoadStaticVerbs (api, CtrlApiVerbs, (void*) CO_Master);
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

    envConfig = getenv("CONTROL_CONFIG_PATH");
    if (!envConfig){
        AFB_API_NOTICE(api, "Using default environnement config path : %s", CONTROL_CONFIG_PATH);
        envConfig = CONTROL_CONFIG_PATH;
    }
    else AFB_API_NOTICE(api, "Found environnement config path : %s", envConfig);


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

    AFB_API_NOTICE(api, "api will be using config : %s", configPath);

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
