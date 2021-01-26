# CANopen Binding

This binding allows the control of a CANopen field network from a redpesk type system. It handles different formats natively (int, float, string...) but can also handle custom formatting using custom decoding/encoding plugins.
It acts as a master on the CANopen bus and is generated and handled via the [(open source industrial c++ library Lely)](https://opensource.lely.com/canopen/).

![CANopen service architecture](images/CANopen-service-architecture.jpg)

To work, this binding requires a master DCF file in which it is described the slaves object dictionary, and a JSON file that describes the behavior of the binding and references the sensors (more information about it can be found in the configuration chapter).

* [Installation Guide](./2-installation_guide.html)
* [Running and Testing](./4-running_and_testing.html)
* [Configuration](./3-configuration.html)
