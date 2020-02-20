/*
 * Copyright (C) 2018 "IoT.bzh"
 * Author "Fulup Ar Foll" <fulup@iot.bzh>
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *	 http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY Kidx, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

//#define _GNU_SOURCE


// #include <lely/io2/posix/fd_loop.hpp>
// #include <lely/io2/linux/can.hpp>
// #include <lely/io2/posix/poll.hpp>
// #include <lely/io2/sys/io.hpp>
// #include <lely/io2/sys/sigset.hpp>
// #include <lely/io2/sys/timer.hpp>
// #include <lely/coapp/fiber_driver.hpp>
// #include <lely/coapp/master.hpp>

#include <iostream>
//#include "CANopen-driver.hpp"

//using namespace std::chrono_literals;
//using namespace lely;

//#include "CANopen-driver.hpp" /*1*/
#include "CANopen-binding.hpp" /*2*/

#include <ctl-config.h>


/*static void lelyEvWatcherCb(sd_event_source* source, uint64_t timer, void *userdata __attribute__((__unused__))){

}*/

void CANopenSensorRequest (afb_req_t request, CANopenSensorT *sensor, json_object *queryJ){

}

void CANopenRtuRequest (afb_req_t request, CANopenRtuT *rtu, json_object *queryJ){

}

int CANopenRtuConnect (afb_api_t api, CANopenRtuT *rtu){

    /*
    CtlConfigT* ctrlConfig = (CtlConfigT*)afb_api_get_userdata(api);
    
    CANopenLelyHandler *coLely = nullptr;
    ctrlConfig->external = coLely;

    lely::io::Context ctx;
    coLely->poll = &lely::io::Poll(ctx);
    coLely->loop = lely::io::FdLoop(coLely->poll);
    auto exec = loop.get_executor();
    coLely->timer = lely::io::Timer(coLely->poll, exec, CLOCK_MONOTONIC);
    coLely->ctrl = lely::io::CanController(rtu->uri);
   
    // Creating a master
    coLely->chan = lely::io::CanChannel(coLely->poll, exec);
    coLely->chan->open(coLely->ctrl);
    if(coLely->chan->is_open()) printf("master: opened virtual CAN channel\n");
    coLely->master = lely::canopen::AsyncMaster(timer, coLely->chan, rtu->dcf, "", 1);
    coLely->driver = MyDriver(exec, coLely->master, rtu->slaveId);
    

    coLely->master->Reset();

    struct sd_event_source* event_source = nullptr;

    auto handler = [](sd_event_source*, int, uint32_t, void* userdata) {
        lely::ev::Poll poll(static_cast<ev_poll_t*>(userdata));
        poll.wait(0);
        return 0;
    };
    auto userdata = const_cast<void*>(static_cast<const void*>(static_cast<ev_poll_t*>(poll.get_poll())));
    sd_event_add_io(afb_daemon_get_event_loop(), &event_source, poll.get_fd(), EPOLLIN, handler, userdata);

    */
    return 0;
}

int CANopenRtuIsConnected (afb_api_t api, CANopenRtuT *rtu){
    return 0;
}
