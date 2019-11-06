ccflags-y += -Wall -Wno-unused-function -Werror -DXLINK_PCIE_LOCAL=1

obj-$(CONFIG_XLINK_PCIE_LH_DRIVER) += mxlk_ep.o
mxlk_ep-objs := mxlk_epf.o
mxlk_ep-objs += mxlk_core.o
mxlk_ep-objs += mxlk_dma.o
mxlk_ep-objs += mxlk_capabilities.o
mxlk_ep-objs += mxlk_interface.o
