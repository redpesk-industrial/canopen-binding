#ifndef _CANOPENSLAVEDRIVER_INCLUDE_
#define _CANOPENSLAVEDRIVER_INCLUDE_

#include <lely/coapp/fiber_driver.hpp>

//#include "CANopenSensor.hpp"
//#include "AglCANopen.hpp"

//#include <queue>
#include <afb/afb-binding>
#include <ctl-config.h>

//*/For Debug
#include <iostream>
//*/

class CANopenSensor;

class CANopenSlaveDriver : public lely::canopen::FiberDriver {
  public:
    using lely::canopen::FiberDriver::FiberDriver;
    CANopenSlaveDriver(
        ev_exec_t* exec,
        lely::canopen::BasicMaster& master,
        afb_api_t api,
        json_object * slaveJ,
        uint8_t nodId
    );
    
    void request (afb_req_t request, json_object * queryJ);

    void addSensorEvent(uint16_t reg, uint8_t subreg, afb_event_t event);

    const char * uid(){
        return m_uid;
    }
    const char * info(){
        return m_info;
    }
    const char * prefix(){
        return m_prefix;
    }
    
    afb_req_t m_current_req;

  private:
    const char * m_uid;
    const char * m_info;
    const char * m_prefix;
    const char * m_dcf;
    uint m_count;
    std::vector<std::shared_ptr<CANopenSensor>> m_sensors;
    struct coEvent{
        uint16_t reg;
        uint8_t subReg;
        afb_event_t event;
    };
    std::vector<coEvent> m_sensorEventQueue;
    
    void OnRpdoWrite(uint16_t idx, uint8_t subidx) noexcept override {
        // std::cout << m_prefix << " ON RPDO WRITE" << std::endl;
        // if (idx == 0x2002 && subidx == 0){
        //     uint16_t val = rpdo_mapped[idx][subidx];
        //     printf("master: received object 2002:00 : %x\n", val);
        //     tpdo_mapped[0x2001][0] = val;
        //     //count= afb_event_push (sensor->event, responseJ);
        // }
        int i = 0;
        for (auto x: m_sensorEventQueue){
            if(idx == x.reg && subidx == x.subReg){
                uint16_t val = rpdo_mapped[idx][subidx];
                afb_event_push (x.event, json_object_new_int((int)val));
            }
            i++;
        }
        std::cout << "m_sensorEventQueue.size = " << m_sensorEventQueue.size() << "donn " << i << "actions" << std::endl;

    }

    void OnBoot(lely::canopen::NmtState nmtState, char es, const ::std::string&) noexcept override {
        std::cout << m_prefix << " ON BOOT" << std::endl;
        if(!es) printf("master: slave #%d successfully booted\n", id());
        master[0x1006][0] = UINT32_C(1000000);
    }

    void OnConfig(::std::function<void(::std::error_code ec)> res) noexcept override {
        std::cout << m_prefix << " ON CONFIG" << std::endl;
        try {
            printf("master: configuring slave #%d\n", (int)id());
            
            //*//On config debug parameters
            Wait(AsyncWrite<uint8_t>(0x6200, 0x01, 0x01));
            Wait(AsyncWrite<uint16_t>(0x1800, 0x05, 0x0000));
            Wait(AsyncWrite<uint32_t>(0x1802, 0x01, 0x80000382));
            // Wait(AsyncWrite<uint32_t>(0x1400, 0x01, 0x00000181));
            //*/


            auto value = Wait(AsyncRead<uint32_t>(0x6200, 0x01));
            std::cout << "On config receved : " <<  value << std::endl;

            res({});
        } catch (lely::canopen::SdoError& e) {
            res(e.code());
        }
    }

    void OnDeconfig(::std::function<void(::std::error_code ec)> res) noexcept override {
        std::cout << m_prefix << " ON DECONFIG" << std::endl;
        printf("master: deconfiguring slave #%d\n", id());
        Wait(AsyncWrite<uint8_t>(0x6200, 0x01, 0x00));
        res({});
    }

    void OnSync(uint8_t cnt, const lely::canopen::DriverBase::time_point&) noexcept override{
        
        //std::cout << m_prefix << " ON SYNC" << std::endl;

        // // Object 2001:00 on the slave was updated by a PDO from the master.
        // uint16_t val = rpdo_mapped[0x2002][0];
        // printf("master: sent PDO with value %d\n", val);
        // // Increment the value for the next SYNC.
        // tpdo_mapped[0x2001][0] = val;
    }
    uint32_t n_{0};
};
#else
#warning "_CANOPENSLAVEDRIVER_INCLUDE_"
#endif /* _CANOPENSLAVEDRIVER_INCLUDE_ */