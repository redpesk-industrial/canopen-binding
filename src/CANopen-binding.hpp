/*
 * Copyright (C) 2016 "IoT.bzh"
 * Author Fulup Ar Foll <fulup@iot.bzh>
 * Author Fulup Ar Foll <romain@iot.bzh>
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */


#ifndef _CANOPEN_BINDING_INCLUDE_
#define _CANOPEN_BINDING_INCLUDE_

// usefull classical include
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <stdbool.h>

#define  AFB_BINDING_VERSION 3
#include <afb/afb-binding>
#include <afb-timer.h>
#include <wrap-json.h>


#ifndef ERROR
  #define ERROR -1
#endif

// hack to get double link rtu<->sensor
typedef struct CANopenSensorS CANopenSensorT;
typedef struct CANopenRtuS CANopenRtuT;
typedef struct CANopenSlaveS CANopenSlaveT;
typedef struct CANopenEncoderCbS CANopenFormatCbT;
typedef struct CANopenSourceS CANopenSourceT;

struct CANopenEncoderCbS {
  const char *uid;
  const char *info;
  const uint nbreg;
  int  subtype;
  int (*encodeCB)(CANopenSourceT *source, struct CANopenEncoderCbS *format, json_object *sourceJ, uint16_t **response, uint index);
  int (*decodeCB)(CANopenSourceT *source, struct CANopenEncoderCbS *format, uint16_t *data, uint index, json_object **responseJ);
  int (*initCB)(CANopenSourceT *source, json_object *argsJ);
};

struct CANopenSourceS {
  const char *sensor;
  afb_api_t api;
  void *context;
};

struct CANopenRtuS {
  const char *uid;
  const char *info;
  const char *uri;
  const char *dcf;
  uint8_t nodId;
  CANopenSlaveT *slaves;
};

struct CANopenSlaveS {
  const char *uid;
  const char *info;
  const char *prefix;
  const char *dcf;
  uint nodId;
  CANopenSensorT *sensors;
  CANopenRtuT *rtu;
  uint count;
};

struct CANopenSensorS {
  const char *uid;
  const char *info;
  const uint registry;
  uint count;
  uint hertz;
  uint iddle;
  uint16_t *buffer;
  CANopenFormatCbT *format;
  //CANopenFunctionCbT *function;
  CANopenSlaveT *slave;
  TimerHandleT *timer;
  afb_api_t api;
  afb_event_t event;
  void *context;
};

/*struct CANopenFunctionCbS {
  const char *uid;
  const char *info;
  CANopenTypeE type;
  int (*readCB) (CANopenSensorT *sensor, json_object **outputJ);
  int (*writeCB)(CANopenSensorT *sensor, json_object *inputJ);
  int (*WReadCB)(CANopenSensorT *sensor, json_object *inputJ, json_object **outputJ);
} ;*/

typedef struct {
  uint16_t *buffer;
  uint count;
  int iddle;
  CANopenSensorT *sensor;
} CANopenEvtT;


// CANopen-glue.c
void CANopenSensorRequest (afb_req_t request, CANopenSensorT *sensor, json_object *queryJ);
void CANopenRtuRequest (afb_req_t request, CANopenRtuT *rtu, json_object *queryJ);
int CANopenRtuConnect (afb_api_t api, CANopenRtuT *rtu);
int CANopenRtuIsConnected (afb_api_t api, CANopenRtuT *rtu);
//CANopenFunctionCbT * coFunctionFind (afb_api_t api, const char *uri);

#endif /* _CANOPEN_BINDING_INCLUDE_ */
