#
# Makefile for the Intel(R) Dynamic Load Balancer 2.0 driver
#

obj-$(CONFIG_INTEL_DLB2) := dlb2.o

dlb2-objs := \
  dlb2_main.o          \
  dlb2_intr.o          \
  dlb2_ioctl.o         \
  dlb2_sriov.o         \
  dlb2_pf_ops.o        \
  dlb2_vf_ops.o        \
  base/dlb2_resource.o \

ifeq ($(CONFIG_INTEL_DLB2_DATAPATH), y)
dlb2-objs += dlb2_dp.o
endif

ifeq ($(CONFIG_INTEL_DLB2_SIOV), y)
dlb2-objs += dlb2_vdcm.o
endif

sse2_instr := $(call as-instr,movapd %xmm0$(comma)%xmm0,-DCONFIG_AS_SSE2=1)
KBUILD_AFLAGS += $(sse2_instr)
KBUILD_CFLAGS += $(sse2_instr)
