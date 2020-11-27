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

#ifndef _ServiceCANopenMaster_INCLUDE_
#define _ServiceCANopenMaster_INCLUDE_

#include <lely/io2/posix/fd_loop.hpp>
#include <lely/io2/linux/can.hpp>
#include <lely/io2/posix/poll.hpp>
#include <lely/io2/sys/io.hpp>
#include <lely/io2/sys/timer.hpp>
#include <lely/coapp/master.hpp>


#include <afb/afb-binding>
#include <ctl-config.h>

class CANopenSlaveDriver;

class CANopenMaster{

  public:
    CANopenMaster(afb_api_t api, json_object *rtuJ, uint8_t nodId = 1);
    inline bool chanIsOpen(){ return m_chan.is_open(); }
    inline bool isRuning(){ return m_isRuning; }
    inline const char * info(){ return m_info; }
    json_object * infoJ();
    json_object * statusJ();
    json_object * slaveListInfo(json_object * array);

  
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
    char * m_dcf = nullptr;
    uint8_t m_nodId;
    // A vector referensing every slaves handle by the master
    std::vector<std::shared_ptr<CANopenSlaveDriver>> m_slaves;
    bool m_isRuning = false;
};

#endif /* _ServiceCANopenMaster_INCLUDE_ */
