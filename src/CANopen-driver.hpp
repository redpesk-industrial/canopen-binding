#ifndef _CANOPEN_DRIVER_INCLUDE_
#define _CANOPEN_DRIVER_INCLUDE_

// #include <lely/io2/posix/fd_loop.hpp>
// #include <lely/io2/linux/can.hpp>
// #include <lely/io2/posix/poll.hpp>
// #include <lely/io2/sys/io.hpp>
// #include <lely/io2/sys/sigset.hpp>
// #include <lely/io2/sys/timer.hpp>
// #include <lely/coapp/fiber_driver.hpp>
// #include <lely/coapp/master.hpp>
// #include <lely/coapp/slave.hpp>

// #include <map>
// #include <array>
// #include <iostream>
// #include <systemd/sd-event.h>
// #include <poll.h>
// #include <afb/afb-binding>

//#define NUM_OP 8



//class CANopenSensor;

// class CANopenSlaveDriver : public lely::canopen::FiberDriver {
//   public:
//     using lely::canopen::FiberDriver::FiberDriver;
//     CANopenSlaveDriver(
//         ev_exec_t* exec,
//         lely::canopen::BasicMaster& master,
//         afb_api_t api,
//         json_object * slaveJ,
//         uint8_t nodId
//     );
    
//     void request (afb_req_t request, json_object * queryJ);

//     const char * uid(){
//         return m_uid;
//     }
//     const char * info(){
//         return m_info;
//     }
//     const char * prefix(){
//         return m_prefix;
//     }
    
//     afb_req_t m_current_req;

//   private:
//     const char * m_uid;
//     const char * m_info;
//     const char * m_prefix;
//     const char * m_dcf;
//     std::map<uint32_t, std::shared_ptr<CANopenSensor>> m_sensors;
//     uint m_count;
    
//     void OnRpdoWrite(uint16_t idx, uint8_t subidx) noexcept override {
//         std::cout << m_prefix << " ON RPDO WRITE" << std::endl;
//         if (idx == 0x2002 && subidx == 0){
//             uint8_t val = rpdo_mapped[idx][subidx];
//             printf("master: received object 2002:00 : %x\n", val);
//         }
//     }

//     void OnBoot(lely::canopen::NmtState nmtState, char es, const ::std::string&) noexcept override {
//         std::cout << m_prefix << " ON BOOT" << std::endl;
//         if(!es) printf("master: slave #%d successfully booted\n", id());
//         master[0x1006][0] = UINT32_C(1000000);
//     }

//     void OnConfig(::std::function<void(::std::error_code ec)> res) noexcept override {
//         std::cout << m_prefix << " ON CONFIG" << std::endl;
//         try {
//             printf("master: configuring slave #%d\n", (int)id());
            
//             //*//On config debug parameters
//             Wait(AsyncWrite<uint8_t>(0x6200, 0x01, 0x01));
//             Wait(AsyncWrite<uint16_t>(0x1800, 0x05, 0x0000));
//             Wait(AsyncWrite<uint32_t>(0x1802, 0x01, 0x80000382));
//             //Wait(AsyncWrite<uint32_t>(0x1400, 0x01, 0x00000181));
//             //*/


//             auto value = Wait(AsyncRead<uint32_t>(0x6200, 0x01));
//             std::cout << "On config receved : " <<  value << std::endl;

//             res({});
//         } catch (lely::canopen::SdoError& e) {
//             res(e.code());
//         }
//     }

//     void OnDeconfig(::std::function<void(::std::error_code ec)> res) noexcept override {
//         std::cout << m_prefix << " ON DECONFIG" << std::endl;
//         printf("master: deconfiguring slave #%d\n", id());
//         Wait(AsyncWrite<uint8_t>(0x6200, 0x01, 0x00));
//         res({});
//     }

//     void OnSync(uint8_t cnt, const lely::canopen::DriverBase::time_point&) noexcept override{
        
//         //std::cout << m_prefix << " ON SYNC" << std::endl;

//         // Object 2001:00 on the slave was updated by a PDO from the master.
//         uint8_t val = rpdo_mapped[0x2002][0];
//         printf("master: sent PDO with value %d\n", val);
//         // Increment the value for the next SYNC.
//         tpdo_mapped[0x2001][0] = val;
//     }
//     uint32_t n_{0};
// };



// class CANopenSensor{
//     public: 
//         CANopenSensor(afb_api_t api, json_object * sensorJ, CANopenSlaveDriver * slaveDriver);
//         void request (afb_req_t request, json_object * queryJ);
//         //~CANopenSensor();
//     private:
//         const char * m_uid;
//         const char * m_info;
//         const uint16_t m_register;
//         const uint8_t m_subRegister;
//         uint m_count;
//         uint m_hertz;
//         uint m_iddle;
//         uint16_t *m_buffer; 
//         // CANopenFormatCbT *m_format;
//         // CANopenFunctionCbT *function;
//         CANopenSlaveDriver *m_slave;
//         // TimerHandleT *m_timer;
//         afb_api_t m_api;
//         afb_event_t m_event;
//         void *m_context;
// };



#endif /* _CANOPEN_DRIVER_INCLUDE_ */