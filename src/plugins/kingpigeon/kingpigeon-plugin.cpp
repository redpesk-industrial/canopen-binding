#define AFB_BINDING_VERSION 3

#include <ctl-config.h>

#include "CANopenSensor.hpp"
#include "CANopenEncoder.hpp"


extern "C"{

    CTLP_CAPI_REGISTER("kingpigeon");

    static int kingpigeon_bool_din4(CANopenSensor* sensor, json_object** outputJ){
        CANopenEncoder::coPDOreadUint8(sensor, outputJ);
        uint8_t val = (uint8_t)json_object_get_int(*outputJ);
        *outputJ = json_object_new_array();
        json_object_array_add(*outputJ, json_object_new_boolean((json_bool)val & 0b00000001));
        json_object_array_add(*outputJ, json_object_new_boolean((json_bool)val & 0b00000010));
        json_object_array_add(*outputJ, json_object_new_boolean((json_bool)val & 0b00000100));
        json_object_array_add(*outputJ, json_object_new_boolean((json_bool)val & 0b00001000));
        return 0;
    }

    static int kingpigeon_percent_ain8(CANopenSensor* sensor, json_object** outputJ){
        CANopenEncoder::coPDOreadUint32(sensor, outputJ);
        uint32_t val = (uint32_t)json_object_get_int64(*outputJ);
        *outputJ = json_object_new_array();
        json_object_array_add(*outputJ, json_object_new_int(val &  0x0000FFFF));
        json_object_array_add(*outputJ, json_object_new_int((val & 0xFFFF0000) >> 16));
        return 0;
    }

    std::map<std::string, CANopenEncodeCbS> kingpigeonRPDO{
        //decignation    decoding CB              encoding CB
        {"kp_bool_din4",{kingpigeon_bool_din4,    nullptr     }},
        {"kp_int_ain2", {kingpigeon_percent_ain8, nullptr     }}
    };

    encodingTableT KingPigeonEncodingTable {
        {"RPDO",{kingpigeonRPDO}}
    };

    CTLP_ONLOAD(plugin, coEncoderHandle) {
        if(!coEncoderHandle)
            return -1;

        CANopenEncoder* coEncoder = (CANopenEncoder*)coEncoderHandle;
        coEncoder->addEncoder(KingPigeonEncodingTable);
        return 0;
    }
}