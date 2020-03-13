#ifndef _AGLCANOPEN_INCLUDE_
#define _AGLCANOPEN_INCLUDE_

#include <lely/io2/posix/fd_loop.hpp>
#include <lely/io2/linux/can.hpp>
#include <lely/io2/posix/poll.hpp>
#include <lely/io2/sys/io.hpp>
//#include <lely/io2/sys/sigset.hpp>
#include <lely/io2/sys/timer.hpp>
//#include <lely/coapp/fiber_driver.hpp>
#include <lely/coapp/master.hpp>
//#include <lely/coapp/slave.hpp>

//#include <map>
//#include <array>
//#include <iostream>
//#include <systemd/sd-event.h>
//#include <poll.h>
#include <afb/afb-binding>
//#include <wrap-json.h>
#include <ctl-config.h>

//#include <vector>

//#include "CANopenSensor.hpp"
//#include "CANopenSlaveDriver.hpp"

class CANopenSlaveDriver;

class AglCANopen{
  public:
    //AglCANopen(const char * uri, const char * dcfFile, uint8_t nodId = 1);
    AglCANopen(afb_api_t api, json_object *rtuJ,sd_event *e, uint8_t nodId = 1);
    
    void addSlave(int slaveId);
    
    bool chanIsOpen(){
        return m_chan.is_open();
    }
    
    bool isRuning(){
        return m_isRuning;
    }
    
    void start();
    
    void start(sd_event *e);

    typedef int (*TypeCB)(int, int);
    static std::map<std::string, TypeCB> avalableTypeCBs;

    ~AglCANopen();
  
  private:
    lely::io::IoGuard m_IoGuard;
    lely::io::Context m_ctx;
    lely::io::Poll m_poll;
    lely::io::FdLoop m_loop;
    lely::ev::Executor m_exec;
    lely::io::Timer m_timer;
    lely::io::CanChannel m_chan;
    std::shared_ptr<lely::io::CanController> m_ctrl;
    std::shared_ptr<lely::canopen::AsyncMaster> m_master;
    const char * m_uid = nullptr;
    const char * m_info = nullptr;
    const char * m_uri = nullptr;
    const char * m_dcf = nullptr;
    uint8_t m_nodId;
    std::vector<std::shared_ptr<CANopenSlaveDriver>> m_slaves;
    bool m_isRuning = false;

    /* FOR BEBUG
    lely::io::CanChannel m_schan;
    std::shared_ptr<MySlave> m_vslaves;
    //*/
};
#else
#warning "_AGLCANOPEN_INCLUDE_"
#endif /* _AGLCANOPEN_INCLUDE_ */