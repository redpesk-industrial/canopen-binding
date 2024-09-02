# Using binary interface

Since version 2.1.0, canopen binding offers a binary interface for querying
and values.

The binary querying exploits typing feature of the bindings.
The querying data must be of the type `canopen-xchg-v1-req`
and must describe the set of data that are queried. The response
will be of type 'canopen-xchg-v1-value' and will hold the
list of values read.

The verb to call is the verb *get* of the api **

A development package called **CANopenXchg** brings to developers
the definition of the C struct to use and a function for recording
types to the binder.

At the moment, to exploit the binary interface, the client binding
must run in the same process as the canopen binding.

Note that this restriction will be removed later when binary links between binders
will become the standard.

## The package CANopenXchg

The package CANopenXchg provides a C header file and a static library for
leveraging binary interface.

It can be required with `pkg-config --cflags --libs CANopenXchg` on CLI.

Within SPEC files, it can be requeried using `BuildRequires: pkgconfig(CANopenXchg)`.

The header must be included this way:

```C
#include <CANopen/CANopenXchg.h>
```

It declares its version. At the moment only the version 1 is
available.

```C
/** current version of CANopenXchg */
#define canopen_xchg_version 1
```

The package CANopenXchg must be initialised by calling the function `canopen_xchg_init`
that returns a status. The returned status must be checked and must be zero
indicating the correct completion.

```C
/** This function declares the types for CANopenXchg */
extern int canopen_xchg_init();
```

That function declares the types used for accessing the binary interface.
At the moment, for the version 1, only two types are made available:

```C
/** type of request data version 1 (valid only after call to canopen_xchg_init) */
extern afb_type_t canopen_xchg_v1_req_type;

/** type of replied data version 1 (valid only after call to canopen_xchg_init) */
extern afb_type_t canopen_xchg_v1_value_type;
```

These types are corresponding to the types of names:

- `canopen_xchg_v1_req_type` is the type of name *"canopen-xchg-v1-req"*
- `canopen_xchg_v1_value_type` is the type of name *"canopen-xchg-v1-value"*


## Querying binary interface using package CANopenXchg

When a client want use the binary interface for requesting one or more values,
it must fill an array of instances of structures `canopen_xchg_v1_req_t`, each
describing one requested value.

Structures `canopen_xchg_v1_req_t` are defined as this:

```C
/**
 * Structure describing one requested value
 * This is the version 1
 */
typedef
struct canopen_xchg_v1_req_s
{
	uint8_t  itf;      /**< index of the interface */
	uint8_t  id;       /**< slave id or 0 for master SDO */
	uint16_t reg;      /**< register index of the PDO mapped value */
	uint8_t  subreg;   /**< sub-register index of the PDO mapped value */
	uint8_t  type;     /**< type of the value (see canopen_xchg_(i|u)(8|16|32|64))*/
	uint8_t  tpdo;     /**< boolean telling if TPDO (not zero) or otherwise RPDO (when zero) */
}
	canopen_xchg_v1_req_t;
```

Each value is identified by:

- the interface number of the CANopen device as set in configuration file
- the id of the equipement or 0 for it self
- the register number within the equipement
- the sub-number within the register
- the type of the expected value as described below
- a boolean indication telling if the value is TPDO

The type of the data must be given as a number using one of the below definition:

```C
/* definition of type indicators (ref. stdint) */
#define canopen_xchg_u8    0  /**< indicator value for type uint8_t */
#define canopen_xchg_i8    1  /**< indicator value for type int8_t */
#define canopen_xchg_u16   2  /**< indicator value for type uint16_t */
#define canopen_xchg_i16   3  /**< indicator value for type int16_t */
#define canopen_xchg_u32   4  /**< indicator value for type uint32_t */
#define canopen_xchg_i32   5  /**< indicator value for type int32_t */
#define canopen_xchg_u64   6  /**< indicator value for type uint64_t */
#define canopen_xchg_i64   7  /**< indicator value for type int64_t */
```

Once initialised, the array of `canopen_xchg_v1_req_t` must be wrapped
in an instance of `afb_data_t` to be passed through the framework
to the binary API of CANopen binding.

If the requests is in the array `requests` of `count` items, the creation
of the data can be achived by calling the below function:

```C
afb_data_t get_req_data(unsigned count, canopen_xchg_v1_req_t *requests, int dofree)
{
  afb_data_t d;
  int rc = afb_create_data_raw(&d, canopen_xchg_v1_req_type,
                               requests, count * sizeof(canopen_xchg_v1_req_t),
			       dofree ? free : NULL, requests);
  return rc == 0 ? d : NULL;
}
```

When the data is created, the verb get can be invoked as below:

```C
	afb_data_t data = get_req_data(count, requests, dofree);
	afb_api_call(myapi, "canopen", "get", 1, &data, onreply, closure);
```

On success, the reply receive one data of type `canopen_xchg_v1_value_type`
that contains as many values as requested, given in a contiguous array
in the order of the query.

Each received value is an instance of the union structure `canopen_xchg_v1_value_t`
defined by:

```C
/**
 * Union structure receiving one requested value
 * This is the version 1
 */
typedef
union canopen_xchg_v1_value_s
{
	uint8_t  u8;   /**< indicator value for type uint8_t */
	int8_t   i8;   /**< indicator value for type int8_t */
	uint16_t u16;  /**< indicator value for type uint16_t */
	int16_t  i16;  /**< indicator value for type int16_t */
	uint32_t u32;  /**< indicator value for type uint32_t */
	int32_t  i32;  /**< indicator value for type int32_t */
	uint64_t u64;  /**< indicator value for type uint64_t */
	int64_t  i64;  /**< indicator value for type int64_t */
}
	canopen_xchg_v1_value_t;
```

The value is the value given by the field of the type requested
in the field `type` of the corresponding query `canopen_xchg_v1_req_t`.

## Advanced usage of the binary interface

When it make sense, for reducing memory allocation/deallocation, the arrays
of values can be allocated by the client. In that case:

- it must be of the same count of elements that the count of queried values,
- it must be passed as the second parameter of the query.

The example below shows such usage:

The sample binding `demex-canopen` uses that feature for implementing
timed events efficiently. Timed events a events that are sent periodically
with the current values of a predefined set of queries. In that case,
the binary interface `get` is always queried with the same set. So it
is an improvement to keep the query data object. But it is also efficient
to keep alive the memory used to for values.

The function `prepare` creates the data for the request and at the same
time the data for the reply
(see [prepare](https://github.com/redpesk-samples/demex-canopen/blob/d5b33ffcf673af69b8e81247a1b871205d799efe/src/demex-canopen.c#L382))

Then the function `on_timed_top` calls the verb `get` with the 2 built
parameters, taking care to add a reference to each instance.
(see [on_timed_top](https://github.com/redpesk-samples/demex-canopen/blob/d5b33ffcf673af69b8e81247a1b871205d799efe/src/demex-canopen.c#L616))

When the reply is received, it must be treated as usual because
there is no guaranty that the recived data is the one passed.
However it can be tested if needed (but doesn't seem very important at end).

