# CANopen Binding

CANopen binding support
TCP Modbus with format conversion for multi-register type as int32, Float, ...

## Dependencies

* AGL application framework 'agl-app-framework-binder-devel'
* AGL controller 'agl-libappcontroller-devel'
* AGL helpers 'agl-libafb-helpers-devel'
* AGL cmake template 'agl-cmake-apps-module'
* lely-core CANopen library

### AGL dependencies

* Declare AGL repository: [(see doc)](https://docs.automotivelinux.org/docs/en/guppy/devguides/reference/2-download-packages.html#install-the-repository)
* Install AGL controller: [(see doc)](https://docs.automotivelinux.org/docs/en/guppy/devguides/reference/ctrler/controller.html)

```bash
# if you did not logout, don't forget to source AGL environnement
source /etc/profile.d/agl-app-framework-binder.sh
```

### CANopen lib dependencies

* Compile and Install fd_loop brach of lely-core CANopen library : [(see doc)](https://gitlab.com/lely_industries/lely-core/-/tree/fd_loop)

## CANopen Binding build

```bash
mkdir build && cd build
cmake ..
make
```

## Running/Testing

Set up your can connection with the right can chanel (usually can0) and a bit rate corresponding to your device setting (in this case 20000).

```bash
sudo ip link set can0 type can bitrate 20000
```

Open your connection :

```bash
sudo ip link set up can0
```

By default on linux, physical can, TX queue length is 10. But lely lib require a minimum TX queue length of 128. you can set it to 1024 to be safe :

```bash
sudo ip link set can0 txqueuelen 1024
```

### Start Sample Binder

Be sure to be in the build directory and run :

```bash
afb-daemon --name=afb-kingpigeonM150-config --port=1234  --ldpaths=src --workdir=. --roothttp=../htdocs --token= --verbose
```

open binding UI with browser at <http://localhost:1234/>

or use `afb-client-demo --human 'ws://localhost:1234/api?token='` to communicate directly with the websocket.

### Adding your own config

Json config file is selected from `afb-daemon --name=afb-midlename-xxx` option. This allows you to switch from one json config to the other without editing any file. `middlename` is use to select a specific config. As example `--name='afb-kpM15-config'` will select `canopen-kpM150-myconfig.json`.

You may also choose to force your config file by exporting CONTROL_CONFIG_PATH environement variable. For further information, check AGL controller documentation [here](https://docs.automotivelinux.org/docs/en/guppy/devguides/reference/ctrler/controllerConfig.html)

```bash
export CONTROL_CONFIG_PATH="$HOME/my-agl-config"
afb-daemon --name=afb-myconfig --port=1234  --ldpaths=src --workdir=. --roothttp=../htdocs --token= --verbose
```

## CANopen binding support a set of default encoder

* int
* uint
* double
* string _--not tested yet_

Nevertheless user may also add its own encoding/decoding format to handle device specific representation (ex: device info string),or custom application encoding (ex: float to uint16 for an analog output or bool array for digital input/output). Custom encoder/decoder are store within user plugin (see sample at src/plugins/kingpigeon).

## API usage

CANopen binding create one api/verb by sensor. By default each sensor api/verb is prefixed by the RTU uid. With following config mak

```json
"canopen":{
    "uid": "CANopen-Master",
    "info": "Master handeling 1 Kingpigeon M150 module", // optional
    "uri" : "can0",
    "nodId": 1, // optional 1 by default
    "dcf": "kp2-master.dcf", // DCF or EDS file describing the behavior of the master and it's handling of the CANopen network
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
            "register" : "0x620001" // sensor located at index 0x6200 sub-index 01
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
            "register" : "0x600001"
          },
    ...
```

## CANopen controller exposed

### Two builtin api/verb

* `api://canopen/ping` check if binder is alive
* `api://canopen/info` return information about the binding configuration and list available verbs

### On action api/verb per declared Sensor

* `api://canopen/mySlave1/mySensor01`
* `api://canopen/mySlave1/mySensor02`
* `api://canopen/mySlave2/mySensor01`
* etc ...

### For each sensors the API accept 4 possible actions

* `action=read` return register(s) value after format decoding
* `action=write` push value on register(s) after format encoding
* `action=subscribe` subscribe to sensors value changes
* `action=unsubscribe` unsubscribe to sensors

### Format Converter

The AGL CANopen support both builtin format converter and optional custom converter provided by user through plugins.

* Standard converter include the traditional int, uint, double ...
* Custom converter are provided through optional plugins. Custom converter should declare a static structure and register it at plugin loadtime(CTLP_ONLOAD).
  * uid is the formatter name as declare inside JSON config file.
  * decode/encore callback are respectively called for read/write action
  * Each sensor stor it's last known value and is accessible with te member function `currentVal()`
  * Each sensor also attaches a void* context accessible with member function `getData()` and `setData()`. Developer may declare a private context for each sensor.

```c++
// Sample of custom formatter (king-pigeon-encore.c)
// -------------------------------------------------

std::map<std::string, coDecodeCB> kingpigeonDecodeFormatersTable{
  //uid              decoding CB
  {"kp_4-boolArray", kingpigeon_4_bool_array_decode},
  {"kp_2-intArray" , kingpigeon_2_int_array_decode }
};

CTLP_ONLOAD(plugin, coEncoderHandle) {
  if(!coEncoderHandle) return -1;
  // get the loaded CANopen Encoder
  CANopenEncoder* coEncoder = (CANopenEncoder*)coEncoderHandle;

  int err;
  
  // add a all list of decode formaters
  err = coEncoder->addDecodeFormateur(kingpigeonDecodeFormatersTable);
  if(err) AFB_API_WARNING(plugin->api, "Kingpigeon-plugin ERROR : fail to add %d entree to decode formater table", err);
  
  // add a single encoder
  err = coEncoder->addEncodeFormateur("kp_4-boolArray",kingpigeon_bool_array_encode);
  if(err) AFB_API_WARNING(plugin->api, "Kingpigeon-plugin ERROR : fail to add 'kp_4-boolArray' entree to encode formater table");
  
  return 0;
}
```
