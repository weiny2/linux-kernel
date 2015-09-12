# Makefile - Intel Omni-Path virtual network controller driver
# Copyright(c) 2015, Intel Corporation.
#
ccflags-y += -I $(src)/../Include
obj-m += opa_vnic.o

opa_vnic-y := opa_vnic_netdev.o opa_vnic_encap.o opa_vnic_ethtool.o \
              opa_vnic_vema.o opa_vnic_debug.o
