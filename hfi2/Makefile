#
# Makefile - Omni-Path HFI PCIe device driver
# Copyright(c) 2014, Intel Corporation.
#
ccflags-y += -I $(src)/../common
ccflags-y += -I $(src)/../common/uapi
ccflags-y += -I $(src)
obj-m += hfi2.o

hfi2-y :=
hfi2-y += debugfs.o
hfi2-y += firmware.o
hfi2-y += link.o
hfi2-y += init.o
hfi2-y += fxr.o
hfi2-y += intr.o
hfi2-y += iommu.o
hfi2-y += pcie.o
hfi2-y += portals.o
hfi2-y += privileged.o
hfi2-y += snoop.o
hfi2-y += error.o
hfi2-y += platform.o
hfi2-y += trace.o
hfi2-y += timesync.o
hfi2-y += verbs/ah.o
hfi2-y += verbs/fxr.o
hfi2-y += verbs/mad.o
hfi2-y += verbs/mr.o
hfi2-y += verbs/qp.o
hfi2-y += verbs/rc.o
hfi2-y += verbs/rdma.o
hfi2-y += verbs/ruc.o
hfi2-y += verbs/uc.o
hfi2-y += verbs/ud.o
hfi2-y += verbs/verbs.o
hfi2-y += verbs/verbs_mcast.o
hfi2-y += verbs/sysfs.o
# TODO - below is temporary
hfi2-y += verbs/rdmavt/vt.o
hfi2-y += verbs/rdmavt/ah.o
hfi2-y += verbs/rdmavt/cq.o
hfi2-y += verbs/rdmavt/dma.o
hfi2-y += verbs/rdmavt/mad.o
hfi2-y += verbs/rdmavt/mcast.o
hfi2-y += verbs/rdmavt/mmap.o
hfi2-y += verbs/rdmavt/mr.o
hfi2-y += verbs/rdmavt/pd.o
hfi2-y += verbs/rdmavt/qp.o
hfi2-y += verbs/rdmavt/srq.o
hfi2-y += verbs/rdmavt/trace.o
