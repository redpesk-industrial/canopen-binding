# Using binary interface

Since version 2.1.0, the canopen binding offers a binary interface for querying values.

At the moment, to exploit the binary interface, the client binding
must run in the same process as the canopen binding.

That restriction ccould be removed later.

## The package CANopenXchg

The package CANopenXchg provides a C header file and a static library for
leveraging binary interface.

It can be required with pkg-config CANopenXchg.

Header can be included this way

```C
#include <CANopen/CANopenXchg.h>
```

## The verb get of canopen

The verb `get` of the API `canopen` accepts one or two arguments.

The first argument is a data of `type canopen_xchg_v1_req_type`
that contains an array of requests. Each request is a
structure of type `canopen_xchg_v1_req_t` defined as below:

```C
typedef
struct canopen_xchg_v1_req_s
{
	uint8_t  itf;      /* index of the interface */
	uint8_t  id;       /* slave id or 0 for master SDO */
	uint16_t reg;      /* register index of the PDO mapped value */
	uint8_t  subreg;   /* sub-register index of the PDO mapped value */
	uint8_t  type;     /* type of the value (see canopen_xchg_(i|u)(8|16|32|64))*/
	uint8_t  tpdo;     /* boolean telling if TPDO (otherwise RPDO) */
}
	canopen_xchg_v1_req_t;
```

The creation of the related data can be done using the following code:

```C
afb_data_t get_req_data(unsigned count, canopen_xchg_v1_req_t *reqs, int dofree)
{
  afb_data_t d;
  afb_create_data_raw(&d, canopen_xchg_v1_req_type, reqs, count * sizeof *reqs, dofree ? free : NULL, reqs);
  return d;
}
```

The verb `get` reads the requested values and returns one data of
type `canopen_xchg_v1_value_type` that contains an array of values
of type `canopen_xchg_v1_value_t`, defined as below:

```C
typedef
union canopen_xchg_v1_value_s
{
	uint8_t  u8;
	int8_t   i8;
	uint16_t u16;
	int16_t  i16;
	uint32_t u32;
	int32_t  i32;
	uint64_t u64;
	int64_t  i64;
}
	canopen_xchg_v1_value_t;
```

When a second argument is given to the request, it must be a data
of type `canopen_xchg_v1_value_type` valid for returning the read values.
