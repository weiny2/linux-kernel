#
# Makefile - Omni-Path HFI PCIe device driver
# Copyright(c) 2014, Intel Corporation.
#
ccflags-y += -I $(src)/../common
ccflags-y += -I $(src)/../common/uapi
obj-m += opa2_hfi.o

opa2_hfi-y :=
opa2_hfi-y += debugfs.o
opa2_hfi-y += init.o
opa2_hfi-y += fxr.o
opa2_hfi-y += intr.o
opa2_hfi-y += iommu.o
opa2_hfi-y += pcie.o
opa2_hfi-y += portals.o
opa2_hfi-y += privileged.o
