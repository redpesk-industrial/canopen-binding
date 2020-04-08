#ifndef _CANOPENSENSOR_INCLUDE_
#define _CANOPENSENSOR_INCLUDE_

#include <afb/afb-binding>
#include <ctl-config.h>
#include "CANopenEncoder.hpp"

class CANopenSlaveDriver;

class CANopenSensor{

    public:
        CANopenSensor(afb_api_t api, json_object * sensorJ, CANopenSlaveDriver * slaveDriver);
        
        // handle request coming from afb
        void request (afb_req_t request, json_object * queryJ);
        
        // Handle read request
        int read(json_object ** inputJ);
        
        // Handle Write request
        int write(json_object *inputJ);
        
        // return information about the slave 
        const char * info();

        inline coEncodeCB encoder(){return m_encode;}
        inline coDecodeCB decoder(){return m_decode;}
        inline const char* uid(){return m_uid;}
        inline afb_event_t event(){return m_event;}
        inline uint16_t reg(){return m_register;}
        inline uint8_t subReg(){return m_subRegister;}
        inline int size(){return m_size;}
        inline CANopenSlaveDriver* slave(){return m_slave;}
        inline COdataType currentVal(){return m_currentVal;}
        inline void * getData(){return m_data;}
        inline void setData(void * data){m_data = data;}
        
    private:
        const char * m_uid;
        const char * m_info = "";
        const char * m_privileges;
        const char * m_format;
        CANopenSlaveDriver* m_slave;
        afb_api_t m_api;
        uint16_t m_register;
        uint8_t m_subRegister;

        // number of Bytes contained by the sensor register
        // 1 = 8bits 2 = 16bits 4 = 32bits 5 = string 
        int m_size;
        afb_event_t m_event = nullptr;

        // set to true if the sensor uses asynchronus read/write function (SDO)
        bool m_asyncSensor = false;

        // read/write callback functions
        CANopenEncodeCbS m_function;
        
        // Formating encoder/decoder callback
        coEncodeCB m_encode;
        coDecodeCB m_decode;

        // store curent state value
        COdataType m_currentVal;

        // available for othe information storing
        void * m_data;

};

#endif /* _CANOPENSENSOR_INCLUDE_ */
