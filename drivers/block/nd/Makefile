obj-$(CONFIG_NFIT_TEST) += test/

ifdef CONFIG_NFIT_TEST
ldflags-y += --wrap=ioremap_cache
ldflags-y += --wrap=iounmap
ldflags-y += --wrap=__request_region
ldflags-y += --wrap=__release_region
endif

obj-$(CONFIG_ND_CORE) += nd_core.o
obj-$(CONFIG_NFIT_ACPI) += nd_acpi.o
obj-$(CONFIG_NFIT_LOCAL_DEV) += local_dev.o
obj-$(CONFIG_NFIT_LOCAL) += local_nfit.o
obj-$(CONFIG_BLK_DEV_PMEM) += nd_pmem.o
obj-$(CONFIG_ND_BTT) += nd_btt.o
obj-$(CONFIG_ND_BLK) += nd_blk.o

local_nfit-y := local.o

nd_acpi-y := acpi.o

nd_core-y := nd.o
nd_core-y += bus.o
nd_core-y += dimm_devs.o
nd_core-y += dimm.o
nd_core-y += region_devs.o
nd_core-y += region.o

nd_core-$(CONFIG_ND_BTT_DEVS) += btt_devs.o

nd_pmem-y := pmem.o
nd_btt-y := btt.o
nd_blk-y := blk.o

obj-$(CONFIG_ND_MANUAL_DSM) += nd_dsm.o
nd_dsm-y := dsm.o
