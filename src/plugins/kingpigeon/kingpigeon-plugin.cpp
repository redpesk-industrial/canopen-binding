//#include "CANopen-driver.hpp"
#include "CANopenSlaveDriver.hpp"
#include <ctl-plugin.h>


CTLP_CAPI_REGISTER("kingpigeon");

// extern "C" {
//     static int kingPigeonOnConf(CANopenSlaveDriver* slave);
//     static int kingPigeonOnBoot(CANopenSlaveDriver* slave);
// }

// static int kingPigeonOnConf(CANopenSlaveDriver* slave){
//     slave->Wait(slave->AsyncWrite<uint8_t>(0x6200, 0x01, 0x01));
//     slave->Wait(slave->AsyncWrite<uint16_t>(0x1800, 0x05, 0x0000));
//     slave->Wait(slave->AsyncWrite<uint32_t>(0x1802, 0x01, 0x80000382));
//     //Wait(AsyncWrite<uint32_t>(0x1400, 0x01, 0x00000181))
// }

// static int kingPigeonOnBoot(CANopenSlaveDriver* slave){
//     slave->Wait(slave->AsyncWrite<uint8_t>(0x6200, 0x01, 0x00));
// }


// CTLP_ONLOAD(plugin, registryCB) {
//     /*registerCbT callback = (registerCbT)registryCB;
//     assert (callback);
//     (*callback) (plugin->uid, pigeonEncoders);*/
//     return 0;
// }