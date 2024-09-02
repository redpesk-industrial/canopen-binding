
## Master DCF generation

A master DCF generator (among others DCF tools) is available in liblely : [dcfgen](https://opensource.lely.com/canopen/docs/dcf-tools/)

The generation can be done at build time if the yml configuration file and the slaves DCF files are stored in a directory named identically to the json configuration file.

The tool `dcfgen` has an important option: the option `--remote-pdo` (alias `-r`) that will generate configuration for the master to map the PDO objects of slaves. This might be important for providing mapping of slave's PDO objects.

For example :

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

