# Use of dcf2afb.py script

The script `dcf2afb.py` scans one DCF or EDS file and
generates a list of sensor definitions for the binding
canopen-binding.

The input to use must be the descriptor of the equipment
item. The product is the content of the `sensors` entry
describing a slave. It can be integrated using reference
expansion: the structures `{"$ref":"PATH"}` are replaced
by the JSON content of the file at PATH by *afb-binder*
at start.


## basis

For its basic use, the script can be run as below:

```sh
$ dcf2afb.py item.dcf
```

When called this way, it outputs the produced result on its
standard output, the screen usually, that can be redirected.

The two below commands are equivalent, they outputs the
produced result to the file named `sensors.json`.

```sh
$ dcf2afb.py item.dcf > sensors.json
$ dcf2afb.py --output sensors.json item.dcf
```

## filtering

The option `--range` selects a range of variables to include
in the product. Variables are selected using their index given
in hexadecimal.

The value is either an index, like `405f` or a range of indexes
like `201f-2050`.

That option can be repeated.

On the below example, the variables of indexes 4000 or in the
range 5600-5fff are selected. The other variables are ignored.

```sh
$ dcf2afb.py --range 4000 --range 5600-5fff item.dcf
```

When the last index is missing, it is replaced by the upper
possible index. So the below command

```sh
$ dcf2afb.py --range 2000- item.dcf
```

selects variable whose index is greater or equal to 0x2000.


## compacting

For each variable of the product, the generator produces
a JSON dictionary as shown in the below example:

```json
   {
      "uid": "4278-cell-fan-status",
      "type": "TPDO",
      "format": "uint",
      "size": 1,
      "register": "0x427800",
      "info": "CellFanStatus (CellFanStatus)"
   }
```

The entry `info` is the original name given in the EDS or DCF file.

The option `--minimal` removes the entry `info`, as shown below:

```json
   {
      "uid": "4278-cell-fan-status",
      "type": "TPDO",
      "format": "uint",
      "size": 1,
      "register": "0x427800"
   }
```

The option `--compact` removes the pretty printing and outputs
(here together with option `--minimal`):

```json
{"uid": "4278-cell-fan-status", "type": "TPDO", "format": "uint", "size": 1, "register": "0x427800"}
```


## managing name scheme

The name of the sensor, its `uid` it computed by the generator
on the basis of the name provided by the original description
(the EDS or DCF file).

Let get the above example for explaining the process.

```json
   {
      "uid": "4278-cell-fan-status",
      "type": "TPDO",
      "format": "uint",
      "size": 1,
      "register": "0x427800",
      "info": "CellFanStatus (CellFanStatus)"
   }
```

The original name is shown at the entry `info`.

It is `CellFanStatus (CellFanStatus)`.

The first thing is to remove parts in parenthesis.
It gives `CellFanStatus  `.

Then non alphanumeric characters are replaced by spaces.
It gives `CellFanStatus  `.

Then camelized words are splitted into their words.
It gives `Cell Fan Status  `.

Then extra spaces are removed and case is lowered.
It gives `cell fan status`.

Then words are possibly shortened. It is not the case here.

Then words are possibly capitalized. It is not the case here.

Then words are joined together using a word separator.
Here it is `-` so the result is `cell-fan-status`.

Then the value is prepended with the index and subindex of
the variable using the index separator.
Here it is `4278` and `-` so the result is `4278-cell-fan-status`.

Options described below allow fine tuning of that process.

The option `--capitalize` if set capitalize the words.

Example:  `--capitalize` gives `4278-Cell-Fan-Status`

The option `--separator` if used tell the separator string
to use for words. It can be of any length and even empty.

Example:  `--separator ::` gives `4278::cell::fan::status`

The option `--separidx` change the separator of index and name.
It can be of any length and even empty. When not set, the word
separator is used.

Example:  `--separidx :` gives `4278:cell-fan-status`

The option `--count` if set to a numeric value restricts the
number of words of the name to that maximum given count, the
rightmost words being discarded when needed.

Example:  `--count 2` gives `4278-cell-fan`

The option `--truncate` if set to a numeric value truncates
the long words to have the given maximum count, the
rightmost characters being discarded when needed.

Example:  `--truncate 3` gives `4278-cel-fan-sta`

The option `--limit` if set to a numeric value tells that
long words are the one with more characters that the given count.
When not set but truncate set, its value is equal the the truncate
value.

Example:  `--truncate 3 --limit 4` gives `4278-cell-fan-sta`

Example of standard variations, using short options:

- camelize: `-c -s ''` gives `4278CellFanStatus`
- cplusize: `-c -s '' -S ::` gives `4278::CellFanStatus`
- snakize: `-s _` gives `4278_cell_fan_status`
- kebabize: (default) `-s -` gives `4278-cell-fan-status`

