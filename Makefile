#
# HFI driver
#
#
ifneq ($(KERNELRELEASE),)
#
# Called from the kernel module build system.
#
obj-m += hfi.o

hfi-y := qib_cq.o qib_diag.o qib_dma.o qib_driver.o qib_eeprom.o \
	qib_file_ops.o qib_fs.o qib_init.o qib_intr.o qib_keys.o \
	qib_mad.o qib_mmap.o qib_mr.o qib_pcie.o qib_pio_copy.o \
	qib_qp.o qib_qsfp.o qib_rc.o qib_ruc.o qib_sdma.o qib_srq.o \
	qib_sysfs.o qib_twsi.o qib_tx.o qib_uc.o qib_ud.o \
	qib_user_pages.o qib_user_sdma.o qib_verbs_mcast.o \
	qib_verbs.o wfr.o

hfi-$(CONFIG_X86_64) += qib_wc_x86_64.o
hfi-$(CONFIG_PPC64) += qib_wc_ppc64.o


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

HDRS= qib_common.h qib.h qib_mad.h qib_qsfp.h qib_user_sdma.h qib_verbs.h 

SRCS= qib_cq.c qib_diag.c qib_dma.c qib_driver.c qib_eeprom.c qib_file_ops.c qib_fs.c qib_init.c qib_intr.c qib_keys.c qib_mad.c qib_mmap.c qib_mr.c qib_pcie.c qib_pio_copy.c qib_qp.c qib_qsfp.c qib_rc.c qib_ruc.c qib_sd7220.c qib_sdma.c qib_srq.c qib_sysfs.c qib_twsi.c qib_tx.c qib_uc.c qib_ud.c qib_user_pages.c qib_user_sdma.c qib_verbs.c qib_verbs_mcast.c qib_wc_ppc64.c qib_wc_x86_64.c wfr.c

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

