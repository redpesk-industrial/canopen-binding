## running the test


sudo ip link set can0 type can bitrate 125000
sudo ip link set up can0
sudo candump can0
sudo /home/jobol/.guix-profile/bin/candump can0


cd tests/test-kp-M150/canopen-kingpigeonM150-config
dcfgen -r gen_master.yml

### server

```
sudo ./run.sh
```

### client

launch it

```
afb-client localhost:1234/api
```

http://localhost:1234/monitoring/

http://localhost:1234/devtools/


run commands

```
canopen ping true
canopen info true

canopen slave2/DIN01_EVENT_TIMER {"action":"write","data":10}
canopen slave2/DIN01_EVENT_TIMER {"action":"write","data":1}
canopen slave2/DIN01 {"action":"read"}
canopen slave2/AIN01 {"action":"read"}
canopen slave2/AIN01 {"action":"SUBSCRIBE"}
canopen slave2/DOUT01 {"action":"write","data":1}
canopen slave2/DOUT01 {"action":"write","data":0}
```


## candump analysis

form: 

```
  can0  602   [8]  40 00 10 00 00 00 00 00
  can0  582   [8]  43 00 10 00 91 01 03 00

  can0  602   [8]  40 18 10 01 00 00 00 00
  can0  582   [8]  43 18 10 01 60 EA 00 00
  
  can0  602   [8]  40 18 10 02 00 00 00 00
  can0  582   [8]  43 18 10 02 96 00 00 00
  
  can0  602   [8]  2B 17 10 00 E8 03 00 00
  can0  582   [8]  60 17 10 00 00 00 00 00
  
  can0  602   [8]  23 00 14 01 02 02 00 80
  can0  582   [8]  60 00 14 01 00 00 00 00
  
  can0  602   [8]  2F 00 14 02 01 00 00 00
  can0  582   [8]  60 00 14 02 00 00 00 00
  
  can0  602   [8]  23 00 14 01 02 02 00 00
  can0  582   [8]  60 00 14 01 00 00 00 00
  
  can0  602   [8]  2B 00 18 05 00 00 00 00
  can0  582   [8]  60 00 18 05 00 00 00 00
  
  can0  602   [8]  2B 02 18 05 00 00 00 00
  can0  582   [8]  60 02 18 05 00 00 00 00
  
  can0  602   [8]  2B 00 00 00 00 00 00 00
  
  can0  602   [8]  80 00 00 00 23 00 0A 06

  can0  582   [8]  80 00 00 00 00 00 02 06
```

602 is SDO master -> slave 2
582 is SDO slave 2 -> master

```
  can0  602   [8]  40 00 10 00 00 00 00 00
  can0  582   [8]  43 00 10 00 91 01 03 00
```

40: ccs=2=upload n=0 e=0 s=0 index=1000 subindex=0
43: ccs=2=upload n=0 e=1 s=1 index=1000 subindex=0

```
  can0  602   [8]  40 18 10 01 00 00 00 00
  can0  582   [8]  43 18 10 01 60 EA 00 00
```

40: ccs=2=upload n=0 e=0 s=0 index=1018 subindex=1
43: ccs=2=upload n=0 e=1 s=1 index=1018 subindex=1

```
  can0  602   [8]  40 18 10 02 00 00 00 00
  can0  582   [8]  43 18 10 02 96 00 00 00
```

40: ccs=2=upload n=0 e=0 s=0 index=1018 subindex=2
43: ccs=2=upload n=0 e=1 s=1 index=1018 subindex=2

```
  can0  602   [8]  2B 17 10 00 E8 03 00 00
  can0  582   [8]  60 17 10 00 00 00 00 00
```

2B: ccs=1=download n=2 e=1 s=1 index=1017 subindex=0
60: ccs=2=seg-upload n=0 e=0 s=0 index=1017 subindex=0


```
  can0  602   [8]  23 00 14 01 02 02 00 80
  can0  582   [8]  60 00 14 01 00 00 00 00
```

23: ccs=1=download n=0 e=1 s=1 index=1400 subindex=1
60: ccs=2=seg-upload n=0 e=0 s=0 index=1400 subindex=1

```
  can0  602   [8]  2F 00 14 02 01 00 00 00
  can0  582   [8]  60 00 14 02 00 00 00 00
```

2F: ccs=1=download n=3 e=1 s=1 index=1400 subindex=2
60: ccs=2=seg-upload n=0 e=0 s=0 index=1400 subindex=2

```
  can0  602   [8]  23 00 14 01 02 02 00 00
  can0  582   [8]  60 00 14 01 00 00 00 00
```
  
23: ccs=1=download n=0 e=1 s=1 index=1400 subindex=1
60: ccs=2=seg-upload n=0 e=0 s=0 index=1400 subindex=1

```
  can0  602   [8]  2B 00 18 05 00 00 00 00
  can0  582   [8]  60 00 18 05 00 00 00 00
```

2B: ccs=1=download n=2 e=1 s=1 index=1800 subindex=5
60: ccs=2=seg-upload n=0 e=0 s=0 index=1800 subindex=5

```
  can0  602   [8]  2B 02 18 05 00 00 00 00
  can0  582   [8]  60 02 18 05 00 00 00 00
```
  
2B: ccs=1=download n=2 e=1 s=1 index=1802 subindex=5
60: ccs=2=seg-upload n=0 e=0 s=0 index=1802 subindex=5

```
  can0  602   [8]  2B 00 00 00 00 00 00 00
  can0  602   [8]  80 00 00 00 23 00 0A 06
  can0  582   [8]  80 00 00 00 00 00 02 06
```

  can0  382   [8]  00 00 00 00 02 00 05 00
