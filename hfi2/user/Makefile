#
# Makefile - Omni-Path HFI character device driver
# Copyright(c) 2014, Intel Corporation.
#
ccflags-y += -I $(src)/../../common/uapi
obj-m += hfi2_user.o

hfi2_user-y :=
hfi2_user-y += device.o
hfi2_user-y += file_ops.o
hfi2_user-y += job.o
hfi2_user-y += mpin.o
hfi2_user-y += ui.o
