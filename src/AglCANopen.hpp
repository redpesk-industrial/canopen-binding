#ifndef _AGLCANOPEN_INCLUDE_
#define _AGLCANOPEN_INCLUDE_

#include <lely/io2/posix/fd_loop.hpp>
#include <lely/io2/linux/can.hpp>
#include <lely/io2/posix/poll.hpp>
#include <lely/io2/sys/io.hpp>
#include <lely/io2/sys/timer.hpp>
#include <lely/coapp/master.hpp>


#include <afb/afb-binding>
#include <ctl-config.h>

class CANopenSlaveDriver;

class AglCANopen{

  public:
    AglCANopen(afb_api_t api, json_object *rtuJ, uint8_t nodId = 1);
    inline bool chanIsOpen(){ return m_chan.is_open(); }
    inline bool isRuning(){ return m_isRuning; }
  
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
    // A vector referensing every slaves handle by the master
    std::vector<std::shared_ptr<CANopenSlaveDriver>> m_slaves;
    bool m_isRuning = false;
};

#endif /* _AGLCANOPEN_INCLUDE_ */
