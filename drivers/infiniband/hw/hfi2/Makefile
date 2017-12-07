#
# Makefile - Omni-Path HFI PCIe device driver
# Copyright(c) 2014, Intel Corporation.
#
ccflags-y += -I $(src)/core
ccflags-y += -I $(src)

obj-$(CONFIG_HFI2_STLNP) += kfi/
obj-$(CONFIG_HFI2_STLNP) += user/

obj-$(CONFIG_INFINIBAND_HFI2) += hfi2.o
hfi2-y :=
hfi2-y += attach.o
hfi2-y += chip.o
hfi2-y += cmdq.o
hfi2-y += counters.o
hfi2-y += debugfs.o
hfi2-y += event.o
hfi2-y += firmware.o
hfi2-y += link.o
hfi2-y += init.o
hfi2-y += intr.o
hfi2-y += iommu.o
hfi2-y += iommu_zebu.o
hfi2-y += pcie.o
hfi2-y += snoop.o
hfi2-y += diag.o
hfi2-y += error.o
hfi2-y += pend_cq.o
hfi2-y += platform.o
hfi2-y += trace.o
hfi2-y += qsfp.o
hfi2-$(CONFIG_HFI2_STLNP) += e2e.o
hfi2-$(CONFIG_HFI2_STLNP) += privileged.o
hfi2-$(CONFIG_HFI2_STLNP) += timesync.o
hfi2-$(CONFIG_HFI2_STLNP) += verbs/native.o
hfi2-$(CONFIG_HFI2_STLNP) += verbs/native_mr.o
hfi2-y += verbs/ah.o
hfi2-y += verbs/hw.o
hfi2-y += verbs/mad.o
hfi2-y += verbs/qp.o
hfi2-y += verbs/rc.o
hfi2-y += verbs/rdma.o
hfi2-y += verbs/ruc.o
hfi2-y += verbs/uc.o
hfi2-y += verbs/ud.o
hfi2-y += verbs/sysfs.o
hfi2-y += verbs/uverbs_device.o
hfi2-y += verbs/uverbs_ctx.o
hfi2-y += verbs/uverbs_cmdq.o
hfi2-y += verbs/uverbs_mmap.o
hfi2-y += verbs/uverbs_job.o
hfi2-y += verbs/verbs.o
hfi2-y += vnic/vnic_main.o
hfi2-y += workarounds.o
