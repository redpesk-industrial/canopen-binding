# CANopen Binding

This binding allows the control of a CANopen field network from a Redpesk type system. It handle different formats natively (int, float, string...) but can also handle custom formatting using custom decoding/encoding plugins.
It act as a master on the CANopen bus and is generated and handled via the [(opensource industrial c++ library Lely)](https://opensource.lely.com/canopen/).

Checkout the documentation from sources on docs folder or on [the redpesk documentation](https://docs.redpesk.bzh/docs/en/master/redpesk-core/canopen/1-architecture_presentation.html).
