#ifndef _CANOPENSENSOR_INCLUDE_
#define _CANOPENSENSOR_INCLUDE_

#include <afb/afb-binding>
#include <ctl-config.h>
#include "CANopen-encoder.hpp"

class CANopenSlaveDriver;

class CANopenSensor{

    public:
        CANopenSensor(afb_api_t api, json_object * sensorJ, CANopenSlaveDriver * slaveDriver);
        void request (afb_req_t request, json_object * queryJ);
        int read(json_object **inputJ);
        int write(json_object *inputJ);

        afb_api_t api();

        inline const char* uid(){return m_uid;}
        inline afb_event_t event(){return m_event;}
        inline uint16_t reg(){return m_register;}
        inline uint8_t subReg(){return m_subRegister;}
        inline CANopenSlaveDriver* slave(){return m_slave;}
        inline void set_ev_q_pos(int qp){ev_q_pos = qp;}

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
        CANopenEncodeCbS m_function;

        int eventCreate();
};

#endif /* _CANOPENSENSOR_INCLUDE_ */
