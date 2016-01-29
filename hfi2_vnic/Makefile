#
# Makefile - Intel Omni-Path Gen2 network driver
# Copyright(c) 2015, Intel Corporation.
#
ccflags-y += -I $(src)/../common
ccflags-y += -I $(src)/../common/uapi
ccflags-y += -I $(src)/vnic.git/Include
obj-m = hfi2_vnic.o

hfi2_vnic-y = hfi2_vnic_main.o

obj-m += vnic.git/opa_vnic_bus/
obj-m += vnic.git/opa_vnic/
