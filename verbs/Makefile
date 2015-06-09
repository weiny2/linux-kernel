#
# Makefile - Omni-Path IB verbs driver
# Copyright(c) 2015, Intel Corporation.
#
ccflags-y += -I $(src)/../common
ccflags-y += -I $(src)/../common/uapi
obj-m += opa2_ib.o

opa2_ib-y := cq.o mad.o mmap.o mr.o pd.o qp.o rdma.o verbs.o
