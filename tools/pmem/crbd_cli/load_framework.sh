#!/bin/sh

if [ "$1" = "" ]
then
 echo "FIT table required"
 exit
fi

mount -t debugfs none /sys/kernel/debug

dmesg -c

modprobe gen_nvdimm
echo "module gen_nvdimm +p" > /sys/kernel/debug/dynamic_debug/control
./crbd_cli --load_fit $1
./crbd_cli --init_dimms
modprobe crbd
echo "module crbd +p" > /sys/kernel/debug/dynamic_debug/control
