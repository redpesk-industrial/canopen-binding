#!/usr/bin/bash

case "$1" in
  tr*) p="strace -f -s100";;
  cgdb|gdb) p="$1 -arg ";;
  serve*) p="gdbserver localhost:9999";;
  vg|valg*) p="valgrind --leak-check=full"; e="-b /home/jobol/redpesk/redpesk-core/afb-binding/tutorials/v4/hello4.so";;
  *) p=exec;;
esac

H=$(realpath $(dirname $0))
#P=${PREFIX:-/usr/local}
#A=$P/redpesk/canopen-binding

L=/home/jobol/.locenv
B=$L/afb
if [[ -d $L/copen ]]
then
	C=$L/copen
	LIBRARY_PATH=$C/lib:$L/helper/lib:$B/lib:$L/base/lib:$L/sec/lib
else
	C=$L/canopen
	LIBRARY_PATH=$C/lib:$L/helper/lib:$B/lib:$L/socle/lib:$L/sec/lib
fi
A=$C/redpesk/canopen-binding


export LIBRARY_PATH
export LD_LIBRARY_PATH=$LIBRARY_PATH
export PATH=$B/bin:$PATH
set -x

export CANOPENPATH=$(realpath $H/../../build/src/plugins/kingpigeon):.
$p afb-binder -M -F -vvvv --rootdir=$A -b $A/lib/CANopen.so:canopen-kingpigeonM150-config.json $e
