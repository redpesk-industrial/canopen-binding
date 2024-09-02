# Configuration

The binding CANopen binding is of the family of bindings that
requires configuration written in JSON.

The configuration until version 1 was a controler V1 configuration.
Since version 2, CANopen binding use the controler of libhelpers
that is an improvement but that keeps compatibility with earlier version.

So, inheriting this history, the name of the API is given in the metadata
of the configuration. But it is safer to always use *canopen* as API name.


## Setting configuration at start

Since version 2, the configuration must be given at start.

Because the CANopen binding is now published as a resource binding,
indicating the configuration can be done in the  *manifest* file
as described in
[resource binding documentation](https://docs.redpesk.bzh/docs/en/master/developer-guides/resource-bindings.html).

It is also possible to set the configuration when running afb-binder in CLI
using either the configuration indicator of option `--binding`
or using the configuration option `--config`.

In all cases, the settings must be of the form:

```json
{
   "set": {
      "CANopen.so": {
         ...
      }
   }
}
```

Where the setting are taking place as the object under `$.set['CANopen.so']`.

Since version 2 there is no more tricky config discovery using binder name and middle tag.

## Main sections of the configuration

The minimal setting is the object:

```json
{
  "metadata": {
    "uid": "canopen",
    "api": "canopen"
  },
  "canopen": DESC
}
```

Where `DESC` holds the configuration in deep.

It is recommended to keep as name of the API *canopen*.
This is the item of path */metadata/api*.

The configuration `DESC` must always be on the path */canopen*.

A full settings can be as rich as:

```json
{
  "metadata": {
    "uid": "canopen",
    "api": "canopen"
    "version": "a version text",
    "info": "some info ....",
    "require": [ "api"... ],
    "author": "me or someonelse or both",
    "date": "a date"
  },
  "plugins": PLUGIN...,
  "onstart": ONSTART...,
  "events": EVENT...,
  "canopen": DESC
}
```

Where `PLUGIN`, `ONSTART` and `EVENT` are defined by ctl-lib of libafb-helpers.

The `PLUGIN` set in the configuration must correspond to CANopen plugins,
see [example](https://github.com/redpesk-industrial/canopen-binding/blob/master/src/plugins/kingpigeon/kingpigeon-plugin.cpp).


## Content describing the API

The content of `DESC` as shown above is either an object describing
the CANopen interface (the connected bus) or when more than one bus
is connected, an array holding the description of each interface (each connected bus).

For each bus (or interface), the object that describes it must have the
fields below:

- `uid`, mandatory string, identifier of the interface

- `uri`, mandatory string, name of the CAN interface, ex: *can0*, *vcan*, ...

- `dcf`, mandatory string, path to the binary DCF file

- `nodId`, mandatory integer, node identifier of the the binding on the bus

- `index`, optional integer, index of the interface for binary API, defaults to zero

- `info`", optional string, some informationnal text

- `slaves`, mandatory, description of the slaves. It is an array of as many
  items as equipment on that bus.

So a basic description looks like:

```json
"canopen": {
    "uid": "an-identifier",
    "info": "some information",
    "uri" : "can0",
    "nodId": 101,
    "dcf": "a-path-to-a-dcf-file",
    "index": 1,
    "slaves": [ SLAVE-A, SLAVE-B, ... ]
}
```

But when more than one CANopen bus exists it looks like:

When more than one CAN interface exists, it is possible to use an array of canopen object description.
The below eluded example gives hint for achieving this.

```json
"canopen": [
    {
        "uid": "Master1",
        "uri" : "can0",
	"index": 1,
	...
    },
    {
        "uid": "Master2",
        "uri" : "can1",
	"index": 2,
	...

    },
    {
        "uid": "MasterX",
        "uri" : "vcan",
	"index": 3,
	...
    },
    ...
]
```

This interface configuration is used for:

1. Setting up the connection to the CAN interface named by `uri`

2. Loading the DCF file named by `dcf` within liblely

3. Declaring VERB endpoints accordingly to `slaves` 

The DCF file contains information for libley about how to set up equipment
of the CANopen bus, if it has to be setup. It also describes the message that
are transmitted on that bus for mapping it to internal drivable structures.

The field `slaves` must hold an array describing each equipment of interest on the bus.

## Content describing a slave

A slave description describes one equipment on the bus.
It allows to attach to it:

- settings to order at startup

- verbs end point for reading and/or writing

A slave description must have the fields below:

- `uid`, mandatory string, an identifier of the slave

- `info`, optional string, an informational text about the slave

- `nodId`, mandatory integer, the identifier of the slave on the bus

- `onconf`, optional array of ONCONF, setting to do at start

- `sensors`, mandatory array of SENSOR, describe the sensors for creation of verbs,
  can be empty array if only binary interface is used.

The value of `uid` is used as base name for creation of verbs.

The value of `nodId` must match the id of the equipment on the bus.

### Setting on start values using onconf

When given, `onconf` must hols an array of settings to be done at start.

Each onconf item of the array is an object having the below fields:

- `info`, optional string, some text
- `register`, mandatory *special integer*, the encoded couple (register, sub-register) to be set
- `size`, mandatory integer, the size ot he data (see below)
- `data`, mandatory *special integer*, the data to be set

The term *special integer* is an integer value that can be given either using
standard decimal notation, or as a string that can be either the decimal notation
or more expectedly the hexadecimal notation using the prefix `0x` or `0X`, ex: "0x450103".

The `size` must de 1, 2 or 4, counting the number of bytes of the value, representing
then values of 8, 16 or 32 bits.

The `data` is the data to be set.

The value of `register` is an integer condensing the register number as upper bits (15..8) and
the sub-register number as lower bits (7..0). For example, to index the sub-register 2 of the
register 0x430, the value of `register` is `"Ox43002". 

### Sensors of slaves

The array of sensors of slaves is used for creating verb endpoints.

Each sensor entry must have the following fields:

- `uid`, mandatory string, an identifier
- `type`, mandatory string, the type of register, SDO, RPDO, TPDO
- `register`, mandatory *special integer*, the encoded couple (register, sub-register) as described above
- `format`, mandatory string, the format: can be "int", "uint", "string" or a special value given by plugin
- `size`, mandatory string, must de 1, 2 or 4, counting the number of bytes of the value, representing
  then values of 8, 16 or 32 bits
- `privilege`, optional string, if given, thi records a permission string that the client must have to access the sensor
- `info`, optional string, some informations
- `args`, optional anything, description of args
- `sample`, optional anything, example of calls

The term *special integer* is an integer value that can be given either using
standard decimal notation, or as a string that can be either the decimal notation
or more expectedly the hexadecimal notation using the prefix `0x` or `0X`, ex: "0x450103".

The `size` must de 1, 2 or 4, counting the number of bytes of the value, representing
then values of 8, 16 or 32 bits.

The format can be "int", "uint", "string" or a format handled by a plugin.

## Generated verbs

Each described sensor has a corresponding verbs that allows to perform some actions
on the sensor, the possible actions depending on the tyupe of the sensor.

Each sensor is described in a json hierarchy looking like:

```json
{
   "canopen": {
      ...,
      "slaves": [
         {
	   "uid": "slave-id",
	   ...,
           "sensors": [
              {
                 "uid": "sensor-id",
		 ...
	      },
	      ...
	   ]
	 },
	 ...
      ]
   }
}
```

It defines verbs of the root API *canopen* named *slave-id/sensor-id*.

The actions are described in the document on API.

## Example


```json
"canopen":{
    "uid": "CANopen-Master",
    "info": "Master handeling 1 Kingpigeon M150 module", // optional
    "uri" : "can0",
    "nodId": 1,
    "dcf": "kp2-master.dcf", // DCF or EDS file describing the behavior of the master and its handling of the CANopen network
    "slaves": [
      {
        "uid": "kp01",
        "info": "Kingpigeon M150", // optional
        "nodId": 2,
        "onconf": [ // optional slave registry setup on pre-operational state
          {
            "info": "Turn off analog input cyclic event",
            "register": "0x180205", // targeted register (in this example : index 0x1802 subindex 05)
            "size" : 2, // number of bytes of the targeted register
            "data": 0
          },
          ...
        ],
        "sensors": [
          {
            "uid": "DIN01",
            "info": "digital input register", // optional
            "type": "RPDO", // type of communication used
            "format" : "kp_bool_din4", // encoding/decoding format
            "size" : 1, //size of the sensor in bytes
            "register" : "0x620001", // sensor located at index 0x6200 sub-index 01
            "sample" : [ // optional : samples that will be available in the afb-ui-devtool
              {"action":"read"},
              {"action":"subscribe"}
            ]
          },
          {
            "uid": "DIN01_EVENT_TIMER",
            "info": "Register to set DIN01 cyclic event in ms (0 fo non)",
            "type": "SDO",
            "format" : "uint",
            "size" : 2,
            "register" : "0x180005",
            "privilege" : "admin" // optional : access to a sensor can require privileges
          },
          {
            "uid": "DOUT01",
            "info": "digital output register",
            "type": "TPDO",
            "format" : "uint",
            "size" : 1,
            "register" : "0x600001",
            "sample" : [
              {"action":"write", "data":15},
              {"action":"write", "data":0}
            ]
          },
    ...
```

## Format Converter

Nevertheless user may also add its own encoding/decoding format to handle device specific representation (ex: device info string),or custom application encoding (ex: float to uint16 for an analog output or bool array for digital input/output). Custom encoder/decoder are stored within user plugin (see sample at src/plugins/kingpigeon).

The CANopen binding support both built-in format converter and optional custom converter provided by user through plugins.

* Standard converter includes the traditional int, uint, double ...
* Custom converter are provided through optional plugins. Custom converter should declare a registering function called `canopenDeclareCodecs` that receives an API handler and a CANopenEncoder object.
* When declaring coder or decoder to a CANopenEncoder object:
  * uid is the formatter name as declare inside JSON config file.
  * decode/encore callback are respectively called for read/write action
  * Each sensor stores its last known value and is accessible with the member function `currentVal()`
  * Each sensor also attaches a void* context accessible with the member function `getData()` and `setData()`. Developer may declare a private context for each sensor.

```C++
// Sample of custom formatter (kingpigeon-plugin.cpp)
// -------------------------------------------------

CTL_PLUGIN_DECLARE("king_pigeon", "CANOPEN plugin for king pigeon");

std::vector<std::pair<std::string, coDecodeCB>> kingpigeonDecodeFormatersTable {
	//uid              decoding CB
	{"kp_4-boolArray", kingpigeon_4_bool_array_decode},
	{"kp_2-intArray", kingpigeon_2_int_array_decode}
};

extern "C"
int canopenDeclareCodecs(afb_api_t api, CANopenEncoder *coEncoder)
{
	// add a all list of decode formaters
	int err = coEncoder->addDecodeFormater(kingpigeonDecodeFormatersTable);
	if (err)
		AFB_API_WARNING(api, "Kingpigeon-plugin ERROR : fail to add %d entree to decode formater table", err);
	// add a single encoder
	err = coEncoder->addEncodeFormater("kp_4-boolArray", kingpigeon_bool_array_encode);
	if (err)
		AFB_API_WARNING(api, "Kingpigeon-plugin ERROR : fail to add 'kp_4-boolArray' entree to encode formater table");

	return 0;
}
```

