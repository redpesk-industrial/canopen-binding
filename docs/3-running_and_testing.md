# Running/Testing

## set up device

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
afb-binder --name=afb-kingpigeonM150-config --port=1234  --binding=src/lib/afb-CANopen.so --workdir=. --roothttp=../htdocs --token= --verbose
```

or if you instaled it (from package or with make install)

```bash
export CANOPEN_BINDING_DIR=/var/local/lib/afm/applications/canopen-binding
afb-binder \
--name=afb-kingpigeonM150-config \
--port=1234 \
--binding=$CANOPEN_BINDING_DIR/lib/afb-CANopen.so \
--roothttp=$CANOPEN_BINDING_DIR/htdocs \
-vvv
```

open binding UI with browser at <http://localhost:1234/>

or use `afb-client --human 'ws://localhost:1234/api?token='` to communicate directly with the websocket.
