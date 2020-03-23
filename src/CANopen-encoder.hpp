#ifndef _CANOPENENCODER_INCLUDE_
#define _CANOPENENCODER_INCLUDE_

#include <map>
#include <iostream>
#include <json-c/json.h>

// #define CO_TYPE_SDO 1
// #define CO_TYPE_TPDO 2
// #define CO_TYPE_RPDO 3


class CANopenSensor;

struct CANopenEncodeCbS {
    int (*readCB)(CANopenSensor* sensor, json_object **inputJ);
    int (*writeCB) (CANopenSensor* sensor, json_object *outputJ);
};

typedef std::map<std::string, std::map<std::string, CANopenEncodeCbS>> encodingTableT;

class CANopenEncoder
{

public:
    //CANopenEncoder();
    int addEncoder(encodingTableT newEncodingTable);
    CANopenEncodeCbS functionCB(std::string type, std::string format);

    static int coSDOwriteUint8(CANopenSensor* sensor, json_object* inputJ);
    static int coSDOwriteUint16(CANopenSensor* sensor, json_object* inputJ);
    static int coSDOwriteUint32(CANopenSensor* sensor, json_object* inputJ);
    static int coSDOreadUint8(CANopenSensor* sensor, json_object** outputJ);
    static int coSDOreadUint16(CANopenSensor* sensor, json_object** outputJ);
    static int coSDOreadUint32(CANopenSensor* sensor, json_object** outputJ);
    static int coPDOwriteUint8(CANopenSensor* sensor, json_object* inputJ);
    static int coPDOwriteUint16(CANopenSensor* sensor, json_object* inputJ);
    static int coPDOwriteUint32(CANopenSensor* sensor, json_object* inputJ);
    static int coPDOreadUint8(CANopenSensor* sensor, json_object** outputJ);
    static int coPDOreadUint16(CANopenSensor* sensor, json_object** outputJ);
    static int coPDOreadUint32(CANopenSensor* sensor, json_object** outputJ);

    //~CANopenEncoder();

private:
    static std::map<std::string, CANopenEncodeCbS> SDOfunctionCBs;
    static std::map<std::string, CANopenEncodeCbS> RPDOfunctionCBs;
    static std::map<std::string, CANopenEncodeCbS> TPDOfunctionCBs;
    //static std::map<std::string, CANopenType> AvalableTypes;
    static encodingTableT encodingTable;
};

// static int coSDOwriteUint8(CANopenSensor* sensor, json_object* inputJ);
// static int coSDOwriteUint16(CANopenSensor* sensor, json_object* inputJ);
// static int coSDOwriteUint32(CANopenSensor* sensor, json_object* inputJ);
// static int coSDOreadUint8(CANopenSensor* sensor, json_object** outputJ);
// static int coSDOreadUint16(CANopenSensor* sensor, json_object** outputJ);
// static int coSDOreadUint32(CANopenSensor* sensor, json_object** outputJ);
// static int coPDOwriteUint8(CANopenSensor* sensor, json_object* inputJ);
// static int coPDOwriteUint16(CANopenSensor* sensor, json_object* inputJ);
// static int coPDOwriteUint32(CANopenSensor* sensor, json_object* inputJ);
// static int coPDOreadUint8(CANopenSensor* sensor, json_object** outputJ);
// static int coPDOreadUint16(CANopenSensor* sensor, json_object** outputJ);
// static int coPDOreadUint32(CANopenSensor* sensor, json_object** outputJ);

// struct CANopenEncodeCbS {
//     int (*readCB) (CANopenSensor* sensor, json_object **inputJ);
//     int (*writeCB)(CANopenSensor* sensor, json_object *outputJ);
// };

// int coSDOwriteUint16(CANopenSensor* sensor, json_object* inputJ);
// int coSDOwriteUint32(CANopenSensor* sensor, json_object* inputJ);
// int coSDOreadUint8(CANopenSensor* sensor, json_object** outputJ);
// int coSDOreadUint16(CANopenSensor* sensor, json_object** outputJ);
// int coSDOreadUint32(CANopenSensor* sensor, json_object** outputJ);
// int coPDOwriteUint8(CANopenSensor* sensor, json_object* inputJ);
// int coPDOwriteUint16(CANopenSensor* sensor, json_object* inputJ);
// int coPDOwriteUint32(CANopenSensor* sensor, json_object* inputJ);
// int coPDOreadUint8(CANopenSensor* sensor, json_object** outputJ);
// int coPDOreadUint16(CANopenSensor* sensor, json_object** outputJ);
// int coSDOwriteUint8(CANopenSensor* sensor, json_object* inputJ);
// int coPDOreadUint32(CANopenSensor* sensor, json_object** outputJ);

#endif //_CANOPENENCODER_INCLUDE_