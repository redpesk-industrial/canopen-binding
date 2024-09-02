# API provided by CANopen

As explained in documentation aboutconfiguration, the name of the API can be changed
in the metadata of the configuration. However it is not safe and we expect here
that the name is always `canopen`.

## Verbs of the root API

The root interface implements 6 verbs: ping, info, status, subscribe, unsubscribe and get.

### Verb ping

This verb takes any parameter and returns it

### Verb info

This verb ignores any incoming parameters and returns a JSON object describing the API.

### Verb status

This verb ignores any incoming parameters and returns a JSON object describing
the current status of the API.

### Verbs subscribe and unsubscribe

These verbs accept no argument or one JSON argument holding either a single string
or an array of strings.

Each string is a regular expression. When nothing is given, the regular expression
`.*` (catch all) is used.

Then the request is subscribed or unsubscribed for each sensor matching any of the
given pattern.

### Verb get

The verb get implements the binary interface and is documented in a specific page.


## Verbs generated per sensors

CANopen binding creates one verb by sensor, the name is `SLAVEID/SENSORID`.

These verbs take one JSON object with the below fields:

- `action`, mandatory string, the required action
- `data`, only when action == write, special integer

The possible actions depend on the type of the sensor.

The table below shows the available matrix:

| action \ type | SDO | RPDO | TPDO |
|---------------|-----|------|------|
| write         | yes | no   | yes  |
| read          | yes | yes  | no   |
| subscribe     | yes | yes  | no   |
| unsubscribe   | yes | yes  | no   |

Only the action read will return a value.