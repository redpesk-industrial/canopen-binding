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

//#include <iostream>
#include <map>
#include <iostream>
//#include <poll.h>

#define NUM_OP 8

class CANopenDriver : public lely::canopen::FiberDriver {
  public:
    using lely::canopen::FiberDriver::FiberDriver;

  private:
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
    AglCANopen(const char * uri, const char * dcfFile, uint8_t nodId = 1);
    int addslave(int slaveId);
    bool chanIsOpen();
    ~AglCANopen();
  
  private:
    lely::io::Context ctx;
    lely::io::Poll poll = lely::io::Poll(ctx);
    lely::io::FdLoop loop = lely::io::FdLoop(poll);
    lely::ev::Executor exec = loop.get_executor();
    lely::io::Timer timer = lely::io::Timer(poll, exec, CLOCK_MONOTONIC);
    lely::io::CanController * ctrl;
    lely::io::CanChannel chan = lely::io::CanChannel(poll, exec);
    lely::canopen::AsyncMaster * master;
    std::map<int, CANopenDriver> Drivers;
};

#endif /* _CANOPEN_DRIVER_INCLUDE_ */