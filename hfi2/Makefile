#
# Makefile - Omni-Path HFI PCIe device driver
# Copyright(c) 2014, Intel Corporation.
#
ccflags-y += -I $(src)/../common
ccflags-y += -I $(src)/../common/uapi
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
hfi2-y += verbs/cq.o
hfi2-y += verbs/dma.o
hfi2-y += verbs/fxr.o
hfi2-y += verbs/mad.o
hfi2-y += verbs/mmap.o
hfi2-y += verbs/mr.o
hfi2-y += verbs/pd.o
hfi2-y += verbs/qp.o
hfi2-y += verbs/rc.o
hfi2-y += verbs/rdma.o
hfi2-y += verbs/ruc.o
hfi2-y += verbs/uc.o
hfi2-y += verbs/ud.o
hfi2-y += verbs/verbs.o
hfi2-y += verbs/verbs_mcast.o
