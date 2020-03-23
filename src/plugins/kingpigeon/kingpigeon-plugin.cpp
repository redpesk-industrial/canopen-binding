#define AFB_BINDING_VERSION 3

#include <ctl-config.h>
//#include "CANopen-driver.hpp"
//#include "CANopenSlaveDriver.hpp"
#include "CANopen-encoder.hpp"


CTLP_CAPI_REGISTER("kingpigeon");

extern "C"{
    static int kingpigeon_bool_din4(CANopenSensor* sensor, json_object** outputJ);
}
    
static int kingpigeon_bool_din4(CANopenSensor* sensor, json_object** outputJ){
    CANopenEncoder::coPDOreadUint16(sensor, outputJ);
    uint16_t val = (uint16_t)json_object_get_int(*outputJ);
    *outputJ = json_object_new_array();
    json_object_array_add(*outputJ, json_object_new_boolean((json_bool)val & 0b00000001));
    json_object_array_add(*outputJ, json_object_new_boolean((json_bool)val & 0b00000010));
    json_object_array_add(*outputJ, json_object_new_boolean((json_bool)val & 0b00000100));
    json_object_array_add(*outputJ, json_object_new_boolean((json_bool)val & 0b00001000));
    return 0;
}

std::map<std::string, CANopenEncodeCbS> kingpigeonRPDO{
    {"bool_din4", {kingpigeon_bool_din4, nullptr}}
};

encodingTableT KingPigeonEncodingTable {
    {"RPDO",{kingpigeonRPDO}}
};

CTLP_ONLOAD(plugin, registryCB) {
    CtlConfigT* ctrlConfig = (CtlConfigT*)afb_api_get_userdata(plugin->api);
    CANopenEncoder* coEncoder = (CANopenEncoder*)ctrlConfig->external;
    coEncoder->addEncoder(KingPigeonEncodingTable);
    return 0;
}
