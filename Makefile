#
# HFI driver
#
#
#
# Called from the kernel module build system.
#
obj-m += hfi.o

hfi-y := cq.o device.o diag.o dma.o driver.o file_ops.o firmware.o fs.o \
	init.o intr.o keys.o mad.o mmap.o mr.o pcie.o pio.o pio_copy.o \
	qp.o qsfp.o rc.o ruc.o sdma.o srq.o sysfs.o trace.o twsi.o tx.o \
	uc.o ud.o user_pages.o user_sdma.o verbs_mcast.o verbs.o wfr.o

CFLAGS_trace.o = -I$(src)


