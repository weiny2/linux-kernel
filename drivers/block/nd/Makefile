obj-$(CONFIG_NFIT_TEST) += test/

ifdef CONFIG_NFIT_TEST
# This obviously will cause symbol collisions if another
# driver/sub-system attempts a similar mocked io-memory implementation.
# When that happens we can either add a 'choice' kconfig option to
# select one mocked instance at a time, or push for the linker to
# include an option of the form "--wrap-prefix=<prefix>" to allow for
# separate namespaces of mocked functions.
ldflags-y += --wrap=ioremap_cache
ldflags-y += --wrap=iounmap
ldflags-y += --wrap=__request_region
ldflags-y += --wrap=__release_region
ldflags-y += --wrap=nd_blk_do_io
ldflags-y += --wrap=devm_ioremap_resource
endif

obj-$(CONFIG_ND_CORE) += nd.o
obj-$(CONFIG_NFIT_ACPI) += nd_acpi.o
obj-$(CONFIG_NFIT_ACPI) += nd_old_acpi.o
obj-$(CONFIG_NFIT_LOCAL_DEV) += local_dev.o
obj-$(CONFIG_NFIT_LOCAL) += local_nfit.o
obj-$(CONFIG_BLK_DEV_PMEM) += nd_pmem.o
obj-$(CONFIG_ND_BTT) += nd_btt.o
obj-$(CONFIG_ND_BLK) += nd_blk.o

local_nfit-y := local.o

nd_acpi-y := acpi.o
nd_old_acpi-y := old_acpi.o

nd-y := core.o
nd-y += bus.o
nd-y += dimm_devs.o
nd-y += dimm.o
nd-y += region_devs.o
nd-y += region.o
nd-y += namespace_devs.o
nd-y += label.o

nd-$(CONFIG_ND_BTT_DEVS) += btt_devs.o

nd_pmem-y := pmem.o
nd_btt-y := btt.o
nd_blk-y := blk.o

obj-$(CONFIG_ND_MANUAL_DSM) += nd_dsm.o
nd_dsm-y := dsm.o
