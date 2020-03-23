// if 2 before 1 => conflict with 'is_error' betwin a lely function and a json define named identically
#include "CANopenSlaveDriver.hpp" /*1*/
#include "CANopenSensor.hpp" /*2*/
#include "CANopen-encoder.hpp"

#ifndef ERROR
    #define ERROR -1
#endif

CANopenEncodeCbS CANopenEncoder::functionCB(std::string type, std::string format){
    
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
                }catch(std::out_of_range){
                    // If function does not existe add it
                    encodingTable.at(typeMap.first).insert(functionMap);
                    std::cout << "DEBUG : Function '" << functionMap.first << "added to encoding table" << std::endl;
                }
            }
        }catch(std::out_of_range){
            // if type does not existe add it
            encodingTable.insert(typeMap);
            std::cout << "DEBUG : type '" << typeMap.first << "added to encoding table" << std::endl;
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
    {"uint8", {CANopenEncoder::coSDOreadUint8, CANopenEncoder::coSDOwriteUint8}},
    {"uint16",{CANopenEncoder::coSDOreadUint16, CANopenEncoder::coSDOwriteUint16}},
    {"uint32",{CANopenEncoder::coSDOreadUint32, CANopenEncoder::coSDOwriteUint32}},
};


std::map<std::string, CANopenEncodeCbS> CANopenEncoder::RPDOfunctionCBs {
    {"uint8", {CANopenEncoder::coPDOreadUint8, nullptr}},
    {"uint16",{CANopenEncoder::coPDOreadUint16, nullptr}},
    {"uint32",{CANopenEncoder::coPDOreadUint32, nullptr}},
};

std::map<std::string, CANopenEncodeCbS> CANopenEncoder::TPDOfunctionCBs {
    {"uint8", {nullptr, CANopenEncoder::coPDOwriteUint8}},
    {"uint16",{nullptr, CANopenEncoder::coPDOwriteUint16}},
    {"uint32",{nullptr, CANopenEncoder::coPDOwriteUint32}},
};

// std::map<std::string, CANopenType> CANopenEncoder::AvalableTypes{
//     {"SDO", CO_SDO},
//     {"TPDO", CO_TPDO},
//     {"RPDO", CO_RPDO}
// };

encodingTableT CANopenEncoder::encodingTable{
    {"SDO", SDOfunctionCBs},
    {"TPDO", TPDOfunctionCBs},
    {"RPDO", RPDOfunctionCBs}
};
