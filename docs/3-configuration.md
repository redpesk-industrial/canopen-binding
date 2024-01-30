# Configuration

## CANopen binding support a set of default encoders

* int
* uint
* double
* string

Nevertheless user may also add its own encoding/decoding format to handle device specific representation (ex: device info string),or custom application encoding (ex: float to uint16 for an analog output or bool array for digital input/output). Custom encoder/decoder are stored within user plugin (see sample at src/plugins/kingpigeon).

## API usage

CANopen binding creates one api/verb by sensor. By default each sensor api/verb is prefixed by the RTU uid. With following config mak

```json
"canopen":{
    "uid": "CANopen-Master",
    "info": "Master handeling 1 Kingpigeon M150 module", // optional
    "uri" : "can0",
    "nodId": 1, // optional 1 by default
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

## CANopen controller exposed

### Two built-in api/verb

* `api://canopen/ping` check if binder is alive
* `api://canopen/info` return information about the binding configuration and available verbs list

### On action api/verb per declared Sensor

* `api://canopen/mySlave1/mySensor01`
* `api://canopen/mySlave1/mySensor02`
* `api://canopen/mySlave2/mySensor01`
* etc ...

### For each sensor the API accepts 4 possible actions

* `action=read` return register(s) value after format decoding
* `action=write` push value on register(s) after format encoding
* `action=subscribe` subscribe to sensors value changes
* `action=unsubscribe` unsubscribe to sensors

### Format Converter

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

## Master DCF generation

A master DCF generator (among others DCF tools) is available in liblely : [dcfgen](https://opensource.lely.com/canopen/docs/dcf-tools/)

The generation can be done at build time if the yml configuration file and the slaves DCF files are stored in a directory named identically to the json configuration file.

for example example :

```bash
project/
└── etc/
    ├── canopen-kingpigeonM150-config/
    │   ├── gen_master.yml
    │   └── my_slave.dcf
    └── canopen-kingpigeonM150-config.json
```

with the folowing gen_master.yml :

```yml
master:
  node_id: 1
  heartbeat_consumer: true
  sync_period: 1000000 # µs

my_slave:
  dcf: "my_slave.dcf"
  node_id: 2
  heartbeat_producer: 1000 # ms
```

will generate :

```bash
package/etc/
├── canopen-kingpigeonM150-config
│   ├── master.bin
│   ├── master.dcf
│   └── my_slave.bin
└── canopen-kingpigeonM150-config.json
```
