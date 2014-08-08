ifdef CONFIG_NFIT_TEST
ldflags-y += --wrap=ioremap_cache
ldflags-y += --wrap=iounmap
endif

obj-$(CONFIG_ND_CORE) += nd-core.o
obj-$(CONFIG_NFIT_ACPI) += nd-acpi.o
obj-$(CONFIG_NFIT_TEST) += nfit-test.o
obj-$(CONFIG_NFIT_TEST) += nfit-test-mem.o

nd-acpi-y := acpi.o
nd-core-y := nd.o bus.o
