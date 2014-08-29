ifdef CONFIG_NFIT_TEST
ldflags-y += --wrap=ioremap_cache
ldflags-y += --wrap=iounmap
ldflags-y += --wrap=__request_region
ldflags-y += --wrap=__release_region
endif

obj-$(CONFIG_ND_CORE) += nd_core.o
obj-$(CONFIG_NFIT_ACPI) += nd_acpi.o
obj-$(CONFIG_NFIT_TEST) += nfit_test.o
obj-$(CONFIG_NFIT_TEST) += nfit_test_mem.o
obj-$(CONFIG_NFIT_REGION) += nd_region.o
obj-$(CONFIG_BLK_DEV_PMEM) += nd_pmem.o

nd_acpi-y := acpi.o
nd_core-y := nd.o bus.o dimm.o
nd_region-y := region.o
nd_pmem-y := pmem.o
