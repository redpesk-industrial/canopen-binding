//to pass lely loop event fd to agl
#include <systemd/sd-event.h>

#include <string.h>

#include "AglCANopen.hpp"
#include "CANopenSlaveDriver.hpp"

static int aglCANopenHandler(sd_event_source*, int, uint32_t, void* userdata) {
    lely::ev::Poll poll(static_cast<ev_poll_t*>(userdata));
    poll.wait(0);
    return 0;
}

const char * fullPathToDCF(afb_api_t api, const char *dcfFile){

    int err = 0;

    char *fullpath = nullptr;
    char *filename = nullptr;

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
    asprintf(&fullpath,"%s/%s",fullpath, dcfFile);
    return fullpath;
}

AglCANopen::AglCANopen(afb_api_t api, json_object *rtuJ, uint8_t nodId /*= 1*/)
    : m_poll{m_ctx}
    , m_loop{m_poll}
    , m_exec {m_loop.get_executor()}
    , m_timer {m_poll, m_exec, CLOCK_MONOTONIC}
    , m_chan {m_poll, m_exec}
{
    int err = 0;
    json_object *slavesJ = NULL;

    assert (rtuJ);
    assert (api);

    err = wrap_json_unpack(rtuJ, "{ss,s?s,ss,s?s,s?i,so !}",
            "uid", &m_uid,
            "info", &m_info,
            "uri", &m_uri,
            "dcf", &m_dcf,
            "nodId", &m_nodId,
            "slaves", &slavesJ);
    if (err) {
        AFB_API_ERROR(api, "Fail to parse rtu JSON : (%s)", json_object_to_json_string(rtuJ));
        return;
    }

    // Find the path of the master description file
    m_dcf = fullPathToDCF(api, m_dcf);
    if (!strlen(m_dcf)){
        AFB_API_ERROR(api, "CANopenLoadOne: fail to find =%s", m_dcf);
        return;
    }
    
    // load and connect CANopen controleur
    try{
        // On linux physical can, the default TX queue length is 10. CanController sets
        // it to 128 if it is too small, which requires the CAP_NET_ADMIN capability
        // (the reason for this is to ensure proper blocking and polling behavior, see
        // section 3.4 in https://rtime.felk.cvut.cz/can/socketcan-qdisc-final.pdf).
        // There are two ways to avoid the need for sudo:
        // * Increase the size of the transmit queue : ip link set can0 txqueuelen 128
        // * Set CanController TX queue length to linux default : CanController ctrl("can0", 10)
        //   but this may cause frames to be dropped.
        m_ctrl = std::make_shared<lely::io::CanController>(m_uri);
    }catch(std::system_error &e){
        AFB_API_ERROR(api, "CANopenLoadOne: fail to connect can uid=%s uri=%s '%s'", m_uid, m_uri, e.what());
        return;
    }
    m_chan.open(*m_ctrl);
    m_master = std::make_shared<lely::canopen::AsyncMaster>(m_timer, m_chan, m_dcf, "", m_nodId);
    if (!m_chan.is_open()) {
        AFB_API_ERROR(api, "CANopenLoadOne: fail to connect can uid=%s uri=%s", m_uid, m_uri);
        return;
    }

    // loop on slaves
    json_object * slaveNodId;
    if (json_object_is_type(slavesJ, json_type_array)) {
        int count = (int)json_object_array_length(slavesJ);
        m_slaves = std::vector<std::shared_ptr<CANopenSlaveDriver>>((size_t)count);
        for (int idx = 0; idx < count; idx++) {
            json_object *slaveJ = json_object_array_get_idx(slavesJ, idx);
            //std::cout << "DEBUG : Slave Json = " << json_object_get_string(slaveJ) << "\n";
            json_object_object_get_ex(slaveJ, "nodId", &slaveNodId);
            m_slaves[idx] = std::make_shared<CANopenSlaveDriver>(m_exec, *m_master, api, slaveJ, json_object_get_int(slaveNodId));
        }
    } else {
        m_slaves = std::vector<std::shared_ptr<CANopenSlaveDriver>>((size_t)1);
        json_object_object_get_ex(slavesJ, "nodId", &slaveNodId);
        m_slaves[0] = std::make_shared<CANopenSlaveDriver>(m_exec, *m_master, api, slavesJ, json_object_get_int(slaveNodId));
    }

    // start the master
    m_master->Reset();

    // Add lely fd to AFB demon event loop
    struct sd_event_source* event_source = nullptr;
    auto userdata = const_cast<void*>(static_cast<const void*>(static_cast<ev_poll_t*>(m_poll.get_poll())));
    err = sd_event_add_io(afb_daemon_get_event_loop(), &event_source, m_poll.get_fd(), EPOLLIN, aglCANopenHandler, userdata);
    if(err == 0) m_isRuning = true;
}

json_object * AglCANopen::infoJ(){
    json_object * responseJ = json_object_new_object();
    char * formatedInfo;
    char isRuningS[7];
    if(m_isRuning) strcpy(isRuningS, "true");
    else strcpy(isRuningS, "false");
    asprintf(&formatedInfo, "uid: '%s', uri: '%s', nodId: %d, isRuning: %s, info: '%s'", m_uid, m_uri, m_nodId, isRuningS, m_info);
    json_object_object_add(responseJ, "Master_info", json_object_new_string(formatedInfo));
    json_object * slavesJ = json_object_new_array();
    for (auto slave: m_slaves){
        json_object_array_add(slavesJ, slave->infoJ());
    }
    json_object_object_add(responseJ, "Slaves", slavesJ);
    return responseJ;
}

// void AglCANopen::reset(){
//     m_master->Reset();
// }
