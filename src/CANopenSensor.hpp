#ifndef _CANOPENSENSOR_INCLUDE_
#define _CANOPENSENSOR_INCLUDE_

#include <afb/afb-binding>
#include <ctl-config.h>


//#include "AglCANopen.hpp"
//#include "CANopenSlaveDriver.hpp"
#include "CANopen-encoder.hpp"

class CANopenSlaveDriver;
//struct CANopenEncodeCbS;

class CANopenSensor{
    
    public: 
        CANopenSensor(afb_api_t api, json_object * sensorJ, CANopenSlaveDriver * slaveDriver);
        void request (afb_req_t request, json_object * queryJ);
        int read(json_object **inputJ);
        int write(json_object *inputJ);
        inline const char* uid(){return m_uid;}
        inline afb_event_t event(){return m_event;}
        inline uint16_t reg(){return m_register;}
        inline uint8_t subReg(){return m_subRegister;}
        inline CANopenSlaveDriver* slave(){return m_slave;}
        inline void set_ev_q_pos(int qp){ev_q_pos = qp;}
        // template <typename T> void sdoWrite(T val);
        // template <typename T> T sdoRead();
        // template <typename T> void pdoWrite(T val);
        // template <typename T> T pdoRead();
        //~CANopenSensor();
    
    private:
        const char * m_uid;
        const char * m_info = "";
        const char * m_privileges;
        CANopenSlaveDriver* m_slave;
        afb_api_t m_api;
        uint16_t m_register;
        uint8_t m_subRegister;
        uint m_count = 0;
        uint m_hertz;
        int m_type;
        afb_event_t m_event;
        int ev_q_pos;
        bool m_asyncSensor = false;
        // struct CANopenEncodeCbS{
        //     int (*readCB) (CANopenSensor* sensor, json_object **inputJ);
        //     int (*writeCB)(CANopenSensor* sensor, json_object *outputJ);
        // } m_function;
        CANopenEncodeCbS m_function;

        int eventCreate();

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

        // static const std::map<std::string, CANopenEncodeCbS> m_SDOfunctionCBs;
        // static const std::map<std::string, CANopenEncodeCbS> m_RPDOfunctionCBs;
        // static const std::map<std::string, CANopenEncodeCbS> m_TPDOfunctionCBs;
        // static const std::map<std::string, uint> m_AvalableTypes;
        // static const std::map<uint, std::map<std::string, CANopenSensor::CANopenEncodeCbS>> m_encodingTable;
};

#endif /* _CANOPENSENSOR_INCLUDE_ */