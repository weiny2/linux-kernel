#
# HFI driver
#
# Build out of tree with:
#	make -C /lib/modules/`uname -r`/build M=`pwd`
#
obj-m += hfi.o

hfi-objs := hfi_init.o

