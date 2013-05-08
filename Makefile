#
# HFI driver
#
#
ifneq ($(KERNELRELEASE),)
#
# Called from the kernel module build system.
#
obj-m += hfi.o

hfi-objs := init.o file_ops.o firmware.o


else
#
# A regular "make".
#
# Useful shell variables:
# KBUILD  - The directory where you built the kernel.  This may be the same
# 	    as the kernel source tree.
# KSOURCE - Used for checkpatch only.  A full source tree.  checkpatch will
#	    not work with <moduledir>/source that you get with a standard
#	    distribution.  You will need to set it manually to a full
#	    kernel source tree.
#
KBUILD  ?= /lib/modules/$(shell uname -r)/build
PWD     := $(shell pwd)

HDRS= hfi.h
SRCS= init.c file_ops.c firmware.c

it:
	$(MAKE) -C $(KBUILD) M=$(PWD)

clean:
	$(MAKE) -C $(KBUILD) M=$(PWD) clean

checkpatch:
	@( [ "$(KSOURCE)" = "" ] && echo "Need to set KSOURCE" && exit 1; \
	   cd $(KSOURCE); \
	   for f in $(HDRS) $(SRCS); do \
		./scripts/checkpatch.pl -f $(PWD)/$$f ; \
	   done \
	)

endif

