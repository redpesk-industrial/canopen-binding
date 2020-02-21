#include "CANopen-driver.hpp"

#include <iostream> //temp


/*void CANopenDriver::OnRpdoWrite(uint16_t idx, uint8_t subidx) {
    if (idx == 0x2002 && subidx == 0){
        uint32_t val = rpdo_mapped[idx][subidx];
        printf("master: received object 2002:00 : %x\n", val);
    }
//uint32_t val = rpdo_mapped[idx][subidx];
//tap_test(val == (n_ > 3 ? n_ - 3 : 0));
}

void CANopenDriver::OnBoot(lely::canopen::NmtState, char es, const ::std::string&) {
    if(!es) printf("master: slave #%d successfully booted\n", id());
    // Start SYNC production.
    master[0x1006][0] = UINT32_C(1000000);
}

void CANopenDriver::OnConfig(::std::function<void(::std::error_code ec)> res) {
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

void CANopenDriver::OnDeconfig(::std::function<void(::std::error_code ec)> res) {
    printf("master: deconfiguring slave #%d\n", id());
    res({});
}

void CANopenDriver::OnSync(uint8_t cnt, const lely::canopen::DriverBase::time_point&) {
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
}*/



AglCANopen::AglCANopen(const char * uri, const char * dcfFile, uint8_t nodId){
    m_ctrl = std::make_shared<lely::io::CanController>(uri);
    m_chan.open(*m_ctrl);
    m_master = std::make_shared<lely::canopen::AsyncMaster>(m_timer, m_chan, dcfFile, "", nodId);
}

void AglCANopen::addSlave(int slaveId){
    m_drivers[slaveId] = std::make_shared<CANopenDriver>(m_exec, *m_master, slaveId);
}

bool AglCANopen::chanIsOpen(){
    return m_chan.is_open();
}

void AglCANopen::start(){
    m_master->Reset();
}

void AglCANopen::start(sd_event *e){
    m_master->Reset();
    struct sd_event_source* event_source = nullptr;
    auto handler = [](sd_event_source*, int, uint32_t, void* userdata) {
        lely::ev::Poll poll(static_cast<ev_poll_t*>(userdata));
        poll.wait(0);
        return 0;
    };
    auto userdata = const_cast<void*>(static_cast<const void*>(static_cast<ev_poll_t*>(m_poll.get_poll())));
    sd_event_add_io(e, &event_source, m_poll.get_fd(), EPOLLIN, handler, userdata);
}

int AglCANopen::m_handler(sd_event_source*, int, uint32_t, void* userdata) {
        lely::ev::Poll poll(static_cast<ev_poll_t*>(userdata));
        poll.wait(0);
        return 0;
    }