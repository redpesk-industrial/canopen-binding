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
    void addSensorEvent(CANopenSensor* sensor);
    void delSensorEvent(CANopenSensor* sensor);

    inline const char * uid() {return m_uid;}
    inline const char * info() {return m_info;}
    inline const char * prefix() {return m_prefix;}

    afb_req_t m_current_req;

  private:
    const char * m_uid;
    const char * m_info;
    const char * m_prefix;
    const char * m_dcf;
    afb_api_t m_api;
    uint m_count;
    std::vector<std::shared_ptr<CANopenSensor>> m_sensors;
    std::list<CANopenSensor*> m_sensorEventQueue;
    json_object *m_onconfJ = nullptr;
    
    void slavePerStartConfig(json_object * conf);

    // This function gets called every time a value is written to the local object dictionary of the master
    void OnRpdoWrite(uint16_t idx, uint8_t subidx) noexcept override {

        int err;
        // check in the sensor event list
        for (auto sensor: m_sensorEventQueue){
            // If the sensor match read it and push the event to afb
            if(idx == sensor->reg() && subidx == sensor->subReg()){
                json_object * responseJ;
                sensor->read(&responseJ);
                err = afb_event_push (sensor->event(), responseJ);
                if(err < 0){
                    AFB_API_ERROR(m_api, "Could not push event from sensor %s", sensor->uid());
                }
            }
        }
    }

    //*// This function gets called when the boot-up process of the slave completes.
    void OnBoot(lely::canopen::NmtState nmtState, char es, const ::std::string&) noexcept override {
        // if master cycle period is null or undefined set it to 10ms
        int val = master[0x1006][0];
        if(val <= 0) master[0x1006][0] = UINT32_C(1000000);
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
