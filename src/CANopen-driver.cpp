#include "CANopen-driver.hpp"
#include <wrap-json.h>
#include <ctl-config.h>

#include <string.h>
//#include <iostream> //temp
#include <net/if.h>
#include <sys/types.h>
//#include <sys/socket.h>
#include <sys/ioctl.h>
//#include <lely/ev/future.hpp>


static int aglCANopenHandler(sd_event_source*, int, uint32_t, void* userdata) {
    lely::ev::Poll poll(static_cast<ev_poll_t*>(userdata));
    poll.wait(0);
    return 0;
}

static int socketTxQline(std::string ifname){
    int s;
    struct ifreq ifr;
    strcpy(ifr.ifr_name, ifname.c_str());
    if((s = socket(PF_CAN, SOCK_RAW, 1)) < 0) {
		std::cout << "Error while opening socket : " << ifname << " not reachable\n";
		return -1;
	}
    if(ioctl(s, SIOCGIFTXQLEN, &ifr)<0){
        std::cout << "Error : Could not assess " << ifname << " informations\n";
        return -1;
    }
    return ifr.ifr_qlen;
}

static void slaveDynRequest(afb_req_t request){
    json_object * queryJ = afb_req_json(request);
    CANopenSlaveDriver * slave = (CANopenSlaveDriver *) afb_req_get_vcbdata(request);
    //slave->Post(slave->request(request, queryJ));
    slave->request(request, queryJ);
}

static void sensorDynRequest(afb_req_t request){

    // retrieve action handle from request and execute the request
    json_object *queryJ = afb_req_json(request);
    CANopenSensor* sensor = (CANopenSensor*) afb_req_get_vcbdata(request);
    sensor->request(request, queryJ);
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
    // strncat(fullpath, "/" , strlen(fullpath));
    // strncat(fullpath, dcfFile , strlen(fullpath));
    return fullpath;
}


AglCANopen::AglCANopen(const char * uri, const char * dcfFile, uint8_t nodId)
    : m_poll{m_ctx}
    , m_loop{m_poll}
    , m_exec {m_loop.get_executor()}
    , m_timer {m_poll, m_exec, CLOCK_MONOTONIC}
    , m_chan {m_poll, m_exec}
    //, m_schan {m_poll, m_exec} //FOR DEBUG
     
{
    m_ctrl = std::make_shared<lely::io::CanController>(uri);
    m_chan.open(*m_ctrl);
    m_master = std::make_shared<lely::canopen::AsyncMaster>(m_timer, m_chan, dcfFile, "", nodId);
}

AglCANopen::AglCANopen(afb_api_t api, json_object *rtuJ,sd_event *e, uint8_t nodId)
    : m_poll{m_ctx}
    , m_loop{m_poll}
    , m_exec {m_loop.get_executor()}
    , m_timer {m_poll, m_exec, CLOCK_MONOTONIC}
    , m_chan {m_poll, m_exec}
    //, m_schan {m_poll, m_exec} //FOR DEBUG
{
    int err = 0;
    //json_object *sensorsJ = NULL;
    json_object *slavesJ = NULL;
    //CtlConfigT *ctrlConfig = nullptr;
    //AglCANopen *CanMaster = nullptr;
    // should already be allocated
    //char *dcf = nullptr;

    assert (rtuJ); 
    assert (api);

    //memset(rtu, 0, sizeof (CANopenRtuT)); // default is empty
    //rtu->nodId = 1;
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
    m_dcf = fullPathToDCF(api, m_dcf);
    if (!strlen(m_dcf)){
        AFB_API_ERROR(api, "CANopenLoadOne: fail to find =%s", m_dcf);
        return;
    }
    //std::cout << "DCF file found : " << dcfFile << std::endl;
    // if uri is provided let's try to connect now
    
    //err = CANopenRtuConnect (api, rtu);
    try{
        // On linux fisical can the default TX queue length is 10. CanController sets
        // it to 128 if it is too small, which requires the CAP_NET_ADMIN capability
        // (the reason for this is to ensure proper blocking and polling behavior, see
        // section 3.4 in https://rtime.felk.cvut.cz/can/socketcan-qdisc-final.pdf).
        // There are two ways to avoid the need for sudo:
        // * Increase the size of the transmit queue : ip link set can0 txqueuelen 128
        // * Set CanController TX queue lengt to linux default : CanController ctrl("can0", 10)
        //   but this may cause frames to be dropped.
        m_ctrl = std::make_shared<lely::io::CanController>(m_uri);
    }catch(std::system_error & e){
        std::cout << e.what() << std::endl;
        int txqlen = socketTxQline(m_uri);
        if( txqlen > -1 && txqlen < 128)
            std::cout << "ERROR : " << m_uri << " TX queue length is curently " << txqlen << ", this is to short. The  minimum requiered is 128\n";
    }

    
    /* FOR DEBUG creating an actual slave
    std::cout << "DEBUG : creating virtual slave !!!!!!!" << std::endl;
    m_schan.open(*m_ctrl);
    if(!m_schan.is_open()){
        AFB_API_ERROR(api, "AglCANopen::AglCANopen: fail to creat virtual slave");
        return;
    }
    const char * slaveDCF = fullPathToDCF(api, "coapp-fiber-slave.dcf");
    if (!strlen(slaveDCF)){
        AFB_API_ERROR(api, "AglCANopen::AglCANopen: fail to find =%s", slaveDCF);
        return;
    }
    m_vslaves = std::make_shared<MySlave>(m_timer, m_schan, slaveDCF, "", 0x02);
    m_vslaves->Reset();
    //*/

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

    //CanMaster->start();
    //this->start(afb_daemon_get_event_loop());
    m_master->Reset();
    struct sd_event_source* event_source = nullptr;
    auto userdata = const_cast<void*>(static_cast<const void*>(static_cast<ev_poll_t*>(m_poll.get_poll())));
    err = sd_event_add_io(afb_daemon_get_event_loop(), &event_source, m_poll.get_fd(), EPOLLIN, aglCANopenHandler, userdata);
    if(err == 0) m_isRuning = true;
}

void AglCANopen::addSlave(int slaveId){
    //m_slaves[slaveId] = std::make_shared<CANopenSlaveDriver>(m_exec, *m_master, slaveId);
}

void AglCANopen::start(){
    m_master->Reset();
}

// void AglCANopen::start(sd_event *e){
//     m_master->Reset();
//     struct sd_event_source* event_source = nullptr;
//     auto handler = [](sd_event_source*, int, uint32_t, void* userdata) {
//         lely::ev::Poll poll(static_cast<ev_poll_t*>(userdata));
//         poll.wait(0);
//         return 0;
//     };
//     auto userdata = const_cast<void*>(static_cast<const void*>(static_cast<ev_poll_t*>(m_poll.get_poll())));
//     int err = sd_event_add_io(e, &event_source, m_poll.get_fd(), EPOLLIN, handler, userdata);
//     if(err == 0) m_isRuning = true;
// }

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
    //CtlConfigT *ctrlConfig = (CtlConfigT*)afb_api_get_userdata(api);
    //AglCANopen *CanMaster = (AglCANopen*)ctrlConfig->external;

    // should already be allocated
    assert (slaveJ);

    //memset(rtu, 0, sizeof (CANopenRtuT)); // default is empty

    // set default values
    //memset(slave, 0, sizeof (CANopenSlaveT));
    //slave->rtu   = rtu;
    //slave->count = 1;
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
    err = afb_api_add_verb(api, m_prefix, m_info, slaveDynRequest, this, nullptr, 0, 0);
    if (err) {
        AFB_API_ERROR(api, "SensorLoadOne: fail to register API verb=%s", m_prefix);
        return;
    } else std::cout << "DEBUG : verb \"" << m_prefix << "\" created !" << "\n";

    // loop on sensors
    if (json_object_is_type(sensorsJ, json_type_array)) {
        int count = (int)json_object_array_length(sensorsJ);
        //m_sensors = (CANopenSensorT*)calloc(count + 1, sizeof (CANopenSensorT));

        for (int idx = 0; idx < count; idx++) {
            json_object *sensorJ = json_object_array_get_idx(sensorsJ, idx);
            m_sensors[idx] = std::make_shared<CANopenSensor>(api, sensorJ, this);
        }

    } else {
        //m_sensors= (CANopenSensorT*) calloc(2, sizeof(CANopenSensorT));
        m_sensors[0] = std::make_shared<CANopenSensor>(api, sensorsJ, this);
    }
    return;
}

void CANopenSlaveDriver::request (afb_req_t request,  json_object * queryJ) {
    
    //json_object *queryJ = afb_req_json(request);
    //CANopenRtuT *rtu = sensor->rtu;
    const char *action;
    json_object *dataJ = nullptr;
    json_object *responseJ = nullptr;
    int idx;
    int subidx;
    int val;
    int size;
    int err;

    /*if (!m_context) {
        afb_req_fail_f(
            request,
            "not-connected",
            "CANopenSlaveDriver::request: RTU not connected rtu=%s sensor=%s query=%s",
            m_slave->uid(),
            m_uid,
            json_object_get_string(queryJ)
        );
        return;
    };*/

    //std::cout << "DEBUG : \"" << m_prefix << "\" have been caled with argument : " << json_object_get_string(queryJ) << "\n";

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

    if (!strcasecmp (action, "WRITE")) {
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
        printf("DEBUG : SDO write to %s[%x][%x] val %x\n with %d bytes", m_prefix, idx, subidx, val, size);
        switch (size)
        {
        case 1:
            AsyncWrite<uint8_t>(idx, subidx, val);
            break;
        case 2:
            AsyncWrite<uint16_t>(idx, subidx, val);
            break;
        case 3:
            AsyncWrite<uint32_t>(idx, subidx, val);
            break;
        case 4:
            AsyncWrite<uint32_t>(idx, subidx, val);
            break;
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

        m_current_req = request;
        afb_req_addref(m_current_req);

        Post([this, request, idx, subidx]() {
            auto v = Wait(AsyncRead<uint32_t>((uint16_t)idx, (uint8_t)subidx));
            std::cout << "Async read of slave " << (int)id() << " [" << std::hex << idx << "]:[" << subidx << "] returned " << v << std::endl;
            afb_req_success(request, json_object_new_int(v), NULL);
            afb_req_unref(request);
        });
        return;

    } /*else if (!strcasecmp (action, "SUBSCRIBE")) {
        err= this->eventCreate (&responseJ);
        if (err) goto OnSubscribeError;
        err=afb_req_subscribe(request, m_event); 
        if (err) goto OnSubscribeError;

    }  else if (!strcasecmp (action, "UNSUBSCRIBE")) {   // Fulup ***** Virer l'event quand le count est à zero
        if (m_event) {
            err=afb_req_unsubscribe(request, m_event); 
            if (err) goto OnSubscribeError;
        }
    }*/
    else {
        afb_req_fail_f (request, "syntax-error", "CANopenSensor::request: action='%s' UNKNOWN rtu=%s query=%s"
            , action, m_uid, json_object_get_string(queryJ));
        return; 
    }
    // everything looks good let's response
    afb_req_success(request, responseJ, NULL);
    return;
}

CANopenSensor::CANopenSensor(afb_api_t api, json_object * sensorJ, CANopenSlaveDriver * slaveDriver)
    : m_register{0x0000}
    , m_subRegister{0x00}
{
    std::cout << "création d'un sensor pour le driver de l'esclave " << (int)slaveDriver->id() << "\n";

    int err = 0;
    const char *type=NULL;
    const char *format=NULL;
    const char *privilege=NULL;
    //afb_auth_t *authent=NULL;
    json_object *argsJ=NULL;
    char* apiverb;
    //CANopenSourceT source;

    // should already be allocated
    assert (sensorJ);

    // set default values
    //memset(sensor, 0, sizeof (CANopenSensorT));
    m_slave = slaveDriver;
    //m_iddle = rtu->iddle;
    m_count = 1;

    err = wrap_json_unpack(sensorJ, "{ss,ss,si,s?s,s?s,s?s,s?i,s?i,s?i,s?o !}",
                "uid", &m_uid,
                "type", &type,
                "register", &m_register,
                "info", &m_info,
                "privilege", &privilege,
                "format", &format,
                "iddle", &m_iddle,
                "count", &m_count,
                "args", &argsJ);
    if (err) {
        AFB_API_ERROR(api, "SensorLoadOne: Fail to parse sensor: %s", json_object_to_json_string(sensorJ));
        return;
    }

    err = asprintf (&apiverb, "%s/%s", m_slave->prefix(), m_uid);
    err = afb_api_add_verb(api, (const char*) apiverb, m_info, sensorDynRequest, this, /*authent*/nullptr, 0, 0);
    if (err) {
        AFB_API_ERROR(api, "SensorLoadOne: fail to register API verb=%s", apiverb);
        return;
    }

    return;  
}

void CANopenSensor::request (afb_req_t request, json_object * queryJ) {
    
    //CANopenRtuT *rtu = sensor->rtu;
    const char *action;
    json_object *dataJ = nullptr;
    json_object *responseJ = nullptr;
    int err;

    if (!m_context) {
        afb_req_fail_f(
            request,
            "not-connected",
            "CANopenSensor::request: RTU not connected rtu=%s sensor=%s query=%s",
            m_slave->uid(),
            m_uid,
            json_object_get_string(queryJ)
        );
        return;
    };

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
        uint val = json_object_get_int(dataJ);
        printf("DEBUG : writing %d on sensor [%x][%x]", val, m_register, m_subRegister);
        m_slave->tpdo_mapped[m_register][m_subRegister] = val;

    } else if (!strcasecmp (action, "READ")) {
        /*if (!m_function->readCB) goto OnReadError;
        err = (m_function->readCB) (this, &responseJ);
        if (err) goto OnReadError;*/
        uint val = m_slave->rpdo_mapped[m_register][m_subRegister];
        printf("DEBUG : writing %d on sensor [%x][%x]", val, m_register, m_subRegister);

    } else if (!strcasecmp (action, "SUBSCRIBE")) {
        /*err= this->eventCreate (&responseJ);
        if (err) goto OnSubscribeError;
        err=afb_req_subscribe(request, m_event); 
        if (err) goto OnSubscribeError;*/

    }  else if (!strcasecmp (action, "UNSUBSCRIBE")) {   // Fulup ***** Virer l'event quand le count est à zero
        /*if (m_event) {
            err=afb_req_unsubscribe(request, m_event); 
            if (err) goto OnSubscribeError;
        }*/
    } else {
        afb_req_fail_f (request, "syntax-error", "CANopenSensor::request: action='%s' UNKNOWN rtu=%s sensor=%s query=%s"
            , action, m_slave->uid(), m_uid, json_object_get_string(queryJ));
        return; 
    }
  
    // everything looks good let's response
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