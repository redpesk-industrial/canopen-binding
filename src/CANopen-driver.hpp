#ifndef _CANOPEN_DRIVER_INCLUDE_
#define _CANOPEN_DRIVER_INCLUDE_

#include <lely/io2/posix/fd_loop.hpp>
#include <lely/io2/linux/can.hpp>
#include <lely/io2/posix/poll.hpp>
#include <lely/io2/sys/io.hpp>
#include <lely/io2/sys/sigset.hpp>
#include <lely/io2/sys/timer.hpp>
#include <lely/coapp/fiber_driver.hpp>
#include <lely/coapp/master.hpp>

#include <map>
#include <array>
#include <iostream>
#include <systemd/sd-event.h>
//#include <poll.h>
#include <afb/afb-binding>

#define NUM_OP 8
#define MAX_CANOPEN_SLAVES 127
#define MAX_CANOPEN_SENSORS 64

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

    const char * uid(){
        return m_uid;
    }
    const char * info(){
        return m_info;
    }
    const char * prefix(){
        return m_prefix;
    }
    
    //uint8_t nodId();

  private:
    const char * m_uid;
    const char * m_info;
    const char * m_prefix;
    const char * m_dcf;
    //uint8_t m_nodId;
    std::map<uint32_t, std::shared_ptr<CANopenSensor>> m_sensors;
    //std::shared_ptr<AglCANopen> m_aglMaster;
    uint m_count;
    
    /*void OnRpdoWrite(uint16_t idx, uint8_t subidx) noexcept override;
    void OnBoot(lely::canopen::NmtState, char es, const ::std::string&) noexcept override;
    void OnConfig(::std::function<void(::std::error_code ec)> res) noexcept override;
    void OnDeconfig(::std::function<void(::std::error_code ec)> res) noexcept override;
    void OnSync(uint8_t cnt, const lely::canopen::DriverBase::time_point&) noexcept override;*/
    void OnRpdoWrite(uint16_t idx, uint8_t subidx) noexcept override {
        if (idx == 0x2002 && subidx == 0){
            uint32_t val = rpdo_mapped[idx][subidx];
            printf("master: received object 2002:00 : %x\n", val);
        }
    //uint32_t val = rpdo_mapped[idx][subidx];
    //tap_test(val == (n_ > 3 ? n_ - 3 : 0));
    }

    void OnBoot(lely::canopen::NmtState, char es, const ::std::string&) noexcept override {
        if(!es) printf("master: slave #%d successfully booted\n", id());
        // Start SYNC production.
        master[0x1006][0] = UINT32_C(1000000);
    }

    void OnConfig(::std::function<void(::std::error_code ec)> res) noexcept override {
        try {
            printf("master: configuring slave #%d\n", id());

            Wait(AsyncWrite<::std::string>(0x2000, 0, "Hello, world!"));
            auto value = Wait(AsyncRead<::std::string>(0x2000, 0));
            std::cout << "On config receved : " <<  value << std::endl;

            res({});
        } catch (lely::canopen::SdoError& e) {
            res(e.code());
        }
    }

    void OnDeconfig(::std::function<void(::std::error_code ec)> res) noexcept override {
        printf("master: deconfiguring slave #%d\n", id());
        res({});
    }

    void OnSync(uint8_t cnt, const lely::canopen::DriverBase::time_point&) noexcept override{
        printf("master: sent SYNC #%d\n", cnt);

        // Object 2001:00 on the slave was updated by a PDO from the master.
        uint32_t val = tpdo_mapped[0x2001][0];
        printf("master: sent PDO with value %d\n", val);
        // Increment the value for the next SYNC.
        tpdo_mapped[0x2001][0] = ++val;

        // Initiate a clean shutdown.
        if (++n_ >= NUM_OP){
            master.AsyncDeconfig(id()).submit(
                GetExecutor(),
                [&]() {
                    master.GetContext().shutdown();
                }
            );
        }
    }
    uint32_t n_{0};
};

class AglCANopen{
  public:
    //AglCANopen();
    AglCANopen(const char * uri, const char * dcfFile, uint8_t nodId = 1);
    AglCANopen(afb_api_t api, json_object *rtuJ,sd_event *e, uint8_t nodId = 1);
    //static int CANopenLoadOne(afb_api_t api, CANopenRtuT *rtu, json_object *rtuJ);
    //void init(const char * uri, const char * dcfFile, uint8_t nodId = 1);
    void addSlave(int slaveId);
    bool chanIsOpen(){
        return m_chan.is_open();
    }
    bool isRuning(){
        return m_isRuning;
    }
    void start();
    void start(sd_event *e);
    ~AglCANopen();
  
  private:
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
    //std::map<int, std::shared_ptr<CANopenSlaveDriver>> m_slaves;
    std::vector<std::shared_ptr<CANopenSlaveDriver>> m_slaves;
    bool m_isRuning = false;
};

class CANopenSensor{
    public: 
        CANopenSensor(afb_api_t api, json_object * sensorJ, CANopenSlaveDriver * slaveDriver);
        void request (afb_req_t request, json_object * queryJ);
        //~CANopenSensor();
    private:
        const char * m_uid;
        const char * m_info;
        const uint16_t m_register;
        const uint8_t m_subRegister;
        uint m_count;
        uint m_hertz;
        uint m_iddle;
        uint16_t *m_buffer; 
        // CANopenFormatCbT *m_format;
        // CANopenFunctionCbT *function;
        CANopenSlaveDriver *m_slave;
        // TimerHandleT *m_timer;
        afb_api_t m_api;
        afb_event_t m_event;
        void *m_context;
};



#endif /* _CANOPEN_DRIVER_INCLUDE_ */