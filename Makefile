#
# HFI driver
#
#
ifneq ($(KERNELRELEASE),)
#
# Called from the kernel module build system.
#
obj-m += hfi.o

hfi-y := cq.o diag.o dma.o driver.o file_ops.o fs.o init.o intr.o keys.o mad.o mmap.o \
	mr.o pcie.o pio_copy.o qp.o qsfp.o rc.o ruc.o sdma.o srq.o sysfs.o twsi.o tx.o \
	uc.o ud.o user_pages.o user_sdma.o verbs_mcast.o verbs.o wfr.o

hfi-$(CONFIG_X86_64) += wc_x86_64.o
hfi-$(CONFIG_PPC64) += wc_ppc64.o


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

HDRS= common.h hfi.h mad.h qsfp.h user_sdma.h verbs.h 

SRCS= cq.c diag.c dma.c driver.c file_ops.c fs.c init.c intr.c keys.c mad.c mmap.c mr.c pcie.c pio_copy.c qp.c qsfp.c rc.c ruc.c sdma.c srq.c sysfs.c twsi.c tx.c uc.c ud.c user_pages.c user_sdma.c verbs.c verbs_mcast.c wc_ppc64.c wc_x86_64.c wfr.c

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

