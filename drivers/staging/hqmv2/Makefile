#
# Makefile for the Intel(R) Hardware Queue Manager v2 driver
#

ccflags-y += -I$(srctree)/drivers/staging/intel/hqmv2

obj-$(CONFIG_INTEL_HQMV2) := hqmv2.o

hqmv2-objs := \
  hqmv2_main.o          \
  hqmv2_intr.o          \
  hqmv2_ioctl.o         \
  hqmv2_sriov.o         \
  hqmv2_pf_ops.o        \
  hqmv2_vf_ops.o        \
  base/hqmv2_resource.o 

ifeq ($(CONFIG_INTEL_HQMV2_DATAPATH), y)
hqmv2-objs += hqmv2_dp.o
endif

#ifeq ($(CONFIG_INTEL_HQMV2_SIOV), y)
#hqmv2-objs += hqmv2_vdcm.o
#endif

sse2_instr := $(call as-instr,movapd %xmm0$(comma)%xmm0,-DCONFIG_AS_SSE2=1)
KBUILD_AFLAGS += $(sse2_instr)
KBUILD_CFLAGS += $(sse2_instr)
