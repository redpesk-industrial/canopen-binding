// if 2 before 1 => conflict with 'is_error' betwin a lely function and a json define named identically
#include "CANopenSlaveDriver.hpp" /*1*/
#include "CANopenSensor.hpp" /*2*/
#include "CANopenEncoder.hpp"

#ifndef ERROR
    #define ERROR -1
#endif

CANopenEncoder::CANopenEncoder() {}

/// @brief Return singleton instance of configuration object.
CANopenEncoder& CANopenEncoder::instance()
{
	static CANopenEncoder encoder;
	return encoder;
}

CANopenEncodeCbS CANopenEncoder::getfunctionCB(std::string type, std::string format){
    CANopenEncodeCbS fn;

    try{
        fn = encodingTable.at(type).at(format);
    } catch(std::out_of_range& e){
        throw e;
    }
    return fn;
}

int CANopenEncoder::addEncoder(encodingTableT newEncodingTable){
    for(auto typeMap : newEncodingTable){
        try{
            // checking for type
            encodingTable.at(typeMap.first);
            for(auto functionMap : typeMap.second){
                try{
                    // Checking for function
                    encodingTable.at(typeMap.first).at(functionMap.first);
                }catch(std::out_of_range&){
                    // If function does not exist add it
                    encodingTable.at(typeMap.first).insert(functionMap);
                }
            }
        }catch(std::out_of_range&){
            // if type does not exist add it
            encodingTable.insert(typeMap);
        }
    }
    return 0;
}

int CANopenEncoder::coSDOwriteUint8(CANopenSensor* sensor, json_object* inputJ){
    int val = json_object_get_int(inputJ);
    sensor->slave()->AsyncWrite<uint8_t>(sensor->reg(), sensor->subReg(), (uint8_t)val);
    return 0;
}

int CANopenEncoder::coSDOwriteUint16(CANopenSensor* sensor, json_object* inputJ){
    int val = json_object_get_int(inputJ);
    sensor->slave()->AsyncWrite<uint16_t>(sensor->reg(), sensor->subReg(), (uint16_t)val);
    return 0;
}

int CANopenEncoder::coSDOwriteUint32(CANopenSensor* sensor, json_object* inputJ){
    int val = json_object_get_int(inputJ);
    sensor->slave()->AsyncWrite<uint32_t>(sensor->reg(), sensor->subReg(), (uint32_t)val);
    return 0;
}

int CANopenEncoder::coSDOwriteString(CANopenSensor* sensor, json_object* inputJ){
    auto val = json_object_get_string(inputJ);
    sensor->slave()->AsyncWrite<::std::string>(sensor->reg(), sensor->subReg(), val);
    return 0;
}

int CANopenEncoder::coSDOreadUint8(CANopenSensor* sensor, json_object** responseJ){
    int val = sensor->slave()->Wait(sensor->slave()->AsyncRead<uint8_t>(sensor->reg(), sensor->subReg()));
    *responseJ = json_object_new_int(val);
    return 0;
}

int CANopenEncoder::coSDOreadUint16(CANopenSensor* sensor, json_object** responseJ){
    int val = sensor->slave()->Wait(sensor->slave()->AsyncRead<uint16_t>(sensor->reg(), sensor->subReg()));
    *responseJ = json_object_new_int(val);
    return 0;
}

int CANopenEncoder::coSDOreadUint32(CANopenSensor* sensor, json_object** responseJ){
    int val = sensor->slave()->Wait(sensor->slave()->AsyncRead<uint32_t>(sensor->reg(), sensor->subReg()));
    *responseJ = json_object_new_int(val);
    return 0;
}

int CANopenEncoder::coSDOreadString(CANopenSensor* sensor, json_object** responseJ){
    auto val = sensor->slave()->Wait(sensor->slave()->AsyncRead<::std::string>(sensor->reg(), sensor->subReg()));
    *responseJ = json_object_new_string(val.c_str());
    return 0;
}

int CANopenEncoder::coPDOwriteUint8(CANopenSensor* sensor, json_object* inputJ){
    sensor->slave()->tpdo_mapped[sensor->reg()][sensor->subReg()] = (uint8_t)json_object_get_int(inputJ);
    return 0;
}

int CANopenEncoder::coPDOwriteUint16(CANopenSensor* sensor, json_object* inputJ){
    sensor->slave()->tpdo_mapped[sensor->reg()][sensor->subReg()] = (uint16_t)json_object_get_int(inputJ);
    return 0;
}

int CANopenEncoder::coPDOwriteUint32(CANopenSensor* sensor, json_object* inputJ){
    sensor->slave()->tpdo_mapped[sensor->reg()][sensor->subReg()] = (uint32_t)json_object_get_int(inputJ);
    return 0;
}

int CANopenEncoder::coPDOreadUint8(CANopenSensor* sensor, json_object** outputJ){
    uint8_t val = sensor->slave()->rpdo_mapped[sensor->reg()][sensor->subReg()];
    *outputJ = json_object_new_int(val);
    return 0;
}

int CANopenEncoder::coPDOreadUint16(CANopenSensor* sensor, json_object** outputJ){
    uint16_t val = sensor->slave()->rpdo_mapped[sensor->reg()][sensor->subReg()];
    *outputJ = json_object_new_int(val);
    return 0;
}

int CANopenEncoder::coPDOreadUint32(CANopenSensor* sensor, json_object** outputJ){
    uint32_t val = sensor->slave()->rpdo_mapped[sensor->reg()][sensor->subReg()];
    *outputJ = json_object_new_int(val);
    return 0;
}


std::map<std::string, CANopenEncodeCbS> CANopenEncoder::SDOfunctionCBs = {
    {"uint8", {coSDOreadUint8, coSDOwriteUint8}},
    {"uint16",{coSDOreadUint16, coSDOwriteUint16}},
    {"uint32",{coSDOreadUint32, coSDOwriteUint32}},
    {"string",{coSDOreadString, coSDOwriteString}}
};

std::map<std::string, CANopenEncodeCbS> CANopenEncoder::RPDOfunctionCBs {
    {"uint8", {coPDOreadUint8, nullptr}},
    {"uint16",{coPDOreadUint16, nullptr}},
    {"uint32",{coPDOreadUint32, nullptr}}
};

std::map<std::string, CANopenEncodeCbS> CANopenEncoder::TPDOfunctionCBs {
    {"uint8", {nullptr, coPDOwriteUint8}},
    {"uint16",{nullptr, coPDOwriteUint16}},
    {"uint32",{nullptr, coPDOwriteUint32}}
};

encodingTableT CANopenEncoder::encodingTable{
    {"SDO", SDOfunctionCBs},
    {"TPDO", TPDOfunctionCBs},
    {"RPDO", RPDOfunctionCBs}
};
