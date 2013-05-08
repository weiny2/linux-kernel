#
# HFI driver
#
# Build out of tree with:
#	make -C /lib/modules/`uname -r`/build M=`pwd`
# Clean with:
#	make -C /lib/modules/`uname -r`/build M=`pwd` clean
#
obj-m += hfi.o

hfi-objs := init.o file_ops.o firmware.o

