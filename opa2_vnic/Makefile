#
# Makefile - Intel Omni-Path Gen2 network driver
# Copyright(c) 2015, Intel Corporation.
#
ccflags-y += -I $(src)/../common
ccflags-y += -I $(src)/../common/uapi
ccflags-y += -I $(src)/Include
obj-m = opa2_vnic_hfi.o

opa2_vnic_hfi-y = opa2_vnic_hfi_main.o

obj-m += opa_vnic_bus/
obj-m += opa_vnic/
