#
# Makefile - Omni-Path HFI character device driver
# Copyright(c) 2014, Intel Corporation.
#
ccflags-y += -mno-fentry
ccflags-y += -I $(src)/../common
ccflags-y += -I $(src)/../common/uapi
obj-m += opa2_user.o

opa2_user-y :=
opa2_user-y += device.o
opa2_user-y += file_ops.o
opa2_user-y += job.o
opa2_user-y += mpin.o
opa2_user-y += ui.o
