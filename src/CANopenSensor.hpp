#ifndef _CANOPENSENSOR_INCLUDE_
#define _CANOPENSENSOR_INCLUDE_

#include <afb/afb-binding>
#include <ctl-config.h>


//#include "AglCANopen.hpp"
//#include "CANopenSlaveDriver.hpp"

class CANopenSlaveDriver;

//template <typename T>
class CANopenSensor{
    
    public: 
        CANopenSensor(afb_api_t api, json_object * sensorJ, CANopenSlaveDriver * slaveDriver);
        void request (afb_req_t request, json_object * queryJ);
        //~CANopenSensor();
    
    private:

        //typedef struct CANopenFunctionCbS CANopenFunctionCbT;


        const char * m_uid;
        //const char * m_prefix;
        const char * m_info = "";
        const char * m_privileges;
        uint16_t m_register;
        uint8_t m_subRegister;
        uint m_count = 0;
        uint m_hertz;
        int m_type;
        //uint m_iddle;
        uint16_t *m_buffer; 
        // CANopenFormatCbT *m_format;
        //CANopenFunctionCbT *m_function;
        CANopenSlaveDriver *m_slave;
        // TimerHandleT *m_timer;
        afb_api_t m_api;
        afb_event_t m_event;
        void *m_context;
        typedef int (*TypeCB)(CANopenSensor*, json_object*);
        TypeCB m_readCB;
        TypeCB m_writeCB;
        
        
        //TypeCB m_TypeCB;

        // T m_readval;
        // T m_writeval;
        // typedef T type;

        // struct CANopenFunctionCbS{
        //     TypeCB readCB;
        //     TypeCB writeCB;
        // };

        struct CANopenFunctionCbS{
            int (*readCB) (CANopenSensor* sensor, json_object **inputJ);
            int (*writeCB)(CANopenSensor* sensor, json_object *outputJ);
        } m_function;

        
        
        //static const std::map<std::string, TypeCB, TypeCB> m_sensorType;


        int eventCreate();

        // static json_object* coEncode_bool(std::string);
        // static json_object* coEncode_uint8(std::string);
        // static json_object* coEncode_uint16(std::string);
        // static json_object* coEncode_uint32(std::string);
        // static json_object* coEncode_uint(std::string);
        // static json_object* coEncode_char(std::string);
        // static json_object* coEncode_short(std::string);
        // static json_object* coEncode_int(std::string);
        // static json_object* coEncode_long(std::string);
        // static json_object* coEncode_double(std::string);
        // static json_object* coEncode_float(std::string);
        // static json_object* coEncode_string(std::string);

        //static int coPDOtype(readCB, writeCB)
        
        static int coSDOwriteUint8(CANopenSensor* sensor, json_object* inputJ);
        static int coSDOwriteUint16(CANopenSensor* sensor, json_object* inputJ);
        static int coSDOwriteUint32(CANopenSensor* sensor, json_object* inputJ);
        static int coSDOreadUint8(CANopenSensor* sensor, json_object** outputJ);
        static int coSDOreadUint16(CANopenSensor* sensor, json_object** outputJ);
        static int coSDOreadUint32(CANopenSensor* sensor, json_object** outputJ);

        static const std::map<std::string, TypeCB> m_avalableReadCBs;
        static const std::map<std::string, TypeCB> m_avalableWriteCBs;

        static const std::map<std::string, CANopenFunctionCbS> m_SDOfunctionCBs;
        static const std::map<std::string, CANopenFunctionCbS> m_RPDOfunctionCBs;
        static const std::map<std::string, CANopenFunctionCbS> m_TPDOfunctionCBs;

        static const std::map<std::string, uint> m_AvalableTypes;
};

#else
#warning "_CANOPENSENSOR_INCLUDE_"
#endif /* _CANOPENSENSOR_INCLUDE_ */