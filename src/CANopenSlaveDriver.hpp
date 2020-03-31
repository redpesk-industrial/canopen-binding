#ifndef _CANOPENSLAVEDRIVER_INCLUDE_
#define _CANOPENSLAVEDRIVER_INCLUDE_

#include <lely/coapp/fiber_driver.hpp>

#include "CANopenSensor.hpp"

#include <afb/afb-binding>
#include <ctl-config.h>
#include <list>

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

    // IMPORTANT : use this funtion only int the driver exec
    int addSensorEvent(CANopenSensor* sensor);
    
    // IMPORTANT : use this funtion only int the driver exec
    int delSensorEvent(CANopenSensor* sensor);

    json_object * infoJ();
    const char * info();

    inline const char * uid() {return m_uid;}

    afb_req_t m_current_req;

  private:
    const char * m_uid;
    const char * m_info;
    const char * m_dcf;
    afb_api_t m_api;
    uint m_count;
    std::vector<std::shared_ptr<CANopenSensor>> m_sensors;
    std::list<CANopenSensor*> m_sensorEventQueue;
    json_object *m_onconfJ = nullptr;
    
    void slavePerStartConfig(json_object * conf);

    // This function gets called every time a value is written to the local object dictionary of the master
    void OnRpdoWrite(uint16_t idx, uint8_t subidx) noexcept override {
        // check in the sensor event list
        for (auto sensor: m_sensorEventQueue){
            // If the sensor match read it and push the event to afb
            if(idx == sensor->reg() && subidx == sensor->subReg()){
                json_object * responseJ;
                sensor->read(&responseJ);
                afb_event_push (sensor->event(), responseJ);
            }
        }
    }

    //*// This function gets called when the boot-up process of the slave completes.
    void OnBoot(lely::canopen::NmtState nmtState, char es, const ::std::string&) noexcept override {
        // if master cycle period is null or undefined set it to 100ms
        int val = master[0x1006][0];
        if(val <= 0) master[0x1006][0] = UINT32_C(100000);
    }//*/

    //*// This function gets called during the boot-up process for the slave.
    void OnConfig(::std::function<void(::std::error_code ec)> res) noexcept override {
        try{
            if(m_onconfJ){
                if (json_object_is_type(m_onconfJ, json_type_array)) {
                    int count = (int)json_object_array_length(m_onconfJ);
                    for (int idx = 0; idx < count; idx++) {
                        json_object *conf = json_object_array_get_idx(m_onconfJ, idx);
                        slavePerStartConfig(conf);
                    }

                } else {
                    slavePerStartConfig(m_onconfJ);
                }
            }
            // Report success (empty error code).
            res({});
        } catch (lely::canopen::SdoError& e) {
            res(e.code());
        }
    }//*/
};

#endif /* _CANOPENSLAVEDRIVER_INCLUDE_ */
