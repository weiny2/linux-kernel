#
# Makefile - Omni-Path HFI PCIe device driver
# Copyright(c) 2014, Intel Corporation.
#
ccflags-y += -I $(src)/../common
ccflags-y += -I $(src)/../common/uapi
obj-m += opa2_hfi.o

opa2_hfi-y :=
opa2_hfi-y += debugfs.o
opa2_hfi-y += firmware.o
opa2_hfi-y += link.o
opa2_hfi-y += init.o
opa2_hfi-y += fxr.o
opa2_hfi-y += intr.o
opa2_hfi-y += iommu.o
opa2_hfi-y += pcie.o
opa2_hfi-y += portals.o
opa2_hfi-y += privileged.o
opa2_hfi-y += attr.o
opa2_hfi-y += verbs/cq.o
opa2_hfi-y += verbs/dma.o
opa2_hfi-y += verbs/fxr.o
opa2_hfi-y += verbs/mad.o
opa2_hfi-y += verbs/mmap.o
opa2_hfi-y += verbs/mr.o
opa2_hfi-y += verbs/pd.o
opa2_hfi-y += verbs/qp.o
opa2_hfi-y += verbs/rc.o
opa2_hfi-y += verbs/rdma.o
opa2_hfi-y += verbs/ruc.o
opa2_hfi-y += verbs/uc.o
opa2_hfi-y += verbs/ud.o
opa2_hfi-y += verbs/verbs.o
opa2_hfi-y += verbs/verbs_mcast.o
