#ifndef _CANOPENENCODER_INCLUDE_
#define _CANOPENENCODER_INCLUDE_

#include <map>
#include <iostream>
#include <json-c/json.h>

class CANopenSensor;

struct CANopenEncodeCbS {
    int (*readCB)(CANopenSensor* sensor, json_object **inputJ);
    int (*writeCB) (CANopenSensor* sensor, json_object *outputJ);
};

typedef std::map<std::string, std::map<std::string, CANopenEncodeCbS>> encodingTableT;

class CANopenEncoder
{
public:
    CANopenEncodeCbS getfunctionCB(std::string type, std::string format);
    static CANopenEncoder& instance();
    int addEncoder(encodingTableT newEncodingTable);

    // available SDO encoding functions
    static int coSDOwriteUint8 (CANopenSensor* sensor, json_object* inputJ);
    static int coSDOwriteUint16(CANopenSensor* sensor, json_object* inputJ);
    static int coSDOwriteUint32(CANopenSensor* sensor, json_object* inputJ);
    static int coSDOwriteString(CANopenSensor* sensor, json_object* inputJ);
    // available SDO decoding functions
    static int coSDOreadUint8 (CANopenSensor* sensor, json_object** outputJ);
    static int coSDOreadUint16(CANopenSensor* sensor, json_object** outputJ);
    static int coSDOreadUint32(CANopenSensor* sensor, json_object** outputJ);
    static int coSDOreadString(CANopenSensor* sensor, json_object** outputJ);
    // available PDO encoding functions
    static int coPDOwriteUint8 (CANopenSensor* sensor, json_object* inputJ);
    static int coPDOwriteUint16(CANopenSensor* sensor, json_object* inputJ);
    static int coPDOwriteUint32(CANopenSensor* sensor, json_object* inputJ);
    // available PDO decoding functions
    static int coPDOreadUint8 (CANopenSensor* sensor, json_object** outputJ);
    static int coPDOreadUint16(CANopenSensor* sensor, json_object** outputJ);
    static int coPDOreadUint32(CANopenSensor* sensor, json_object** outputJ);

private:
    CANopenEncoder(); ///< Private constructor for singleton implementation

    static std::map<std::string, CANopenEncodeCbS> SDOfunctionCBs;
    static std::map<std::string, CANopenEncodeCbS> RPDOfunctionCBs;
    static std::map<std::string, CANopenEncodeCbS> TPDOfunctionCBs;
    static encodingTableT encodingTable;
};

#endif //_CANOPENENCODER_INCLUDE_
