// SPDX-License-Identifier: GPL-2.0 only
/*****************************************************************************
 *
 * Intel Keem Bay XLink PCIe Driver
 *
 * Copyright (C) 2019 Intel Corporation
 *
 ****************************************************************************/

#include "mxlk_dma.h"
#include "mxlk_struct.h"
#include "../common/mxlk.h"
#include "../common/mxlk_mmio.h"

#define DMA_DBI_OFFSET (0x380000)
#define DMA_WRITE_CHAN0_OFFSET (0x200)
#define DMA_READ_CHAN0_OFFSET (0x300)

/* PCIe DMA control 1 register definitions. */
#define DMA_CH_CONTROL1_LIE_SHIFT (3)
#define DMA_CH_CONTROL1_LIE_MASK (BIT(DMA_CH_CONTROL1_LIE_SHIFT))

#define DMA_DEF_CHANNEL (0)

/* PCIe DMA Engine enable regsiter definitions. */
#define DMA_ENGINE_EN_SHIFT (0)
#define DMA_ENGINE_EN_MASK (BIT(DMA_ENGINE_EN_SHIFT))

/* PCIe DMA interrupt registers defintions. */
#define DMA_ABORT_INTERRUPT_SHIFT (16)

#define DMA_ABORT_INTERRUPT_MASK (0xFF << DMA_ABORT_INTERRUPT_SHIFT)
#define DMA_ABORT_INTERRUPT_CH_MASK(_c) (BIT(_c) << DMA_ABORT_INTERRUPT_SHIFT)
#define DMA_DONE_INTERRUPT_MASK (0xFF)
#define DMA_DONE_INTERRUPT_CH_MASK(_c) (BIT(_c))
#define DMA_ALL_INTERRUPT_MASK                                            \
	(DMA_ABORT_INTERRUPT_MASK | DMA_DONE_INTERRUPT_MASK)

#define DMA_LL_ERROR_SHIFT (16)
#define DMA_CPL_ABORT_SHIFT (8)
#define DMA_CPL_TIMEOUT_SHIFT (16)
#define DMA_DATA_POI_SHIFT (24)
#define DMA_AR_ERROR_CH_MASK(_c) (BIT(_c))
#define DMA_LL_ERROR_CH_MASK(_c) (BIT(_c) << DMA_LL_ERROR_SHIFT)
#define DMA_UNREQ_ERROR_CH_MASK(_c) (BIT(_c))
#define DMA_CPL_ABORT_ERROR_CH_MASK(_c) (BIT(_c) << DMA_CPL_ABORT_SHIFT)
#define DMA_CPL_TIMEOUT_ERROR_CH_MASK(_c) (BIT(_c) << DMA_CPL_TIMEOUT_SHIFT)
#define DMA_DATA_POI_ERROR_CH_MASK(_c) (BIT(_c) << DMA_DATA_POI_SHIFT)

struct __packed pcie_dma_reg {
	u32 dma_ctrl_data_arb_prior;
	u32 reserved1;
	u32 dma_ctrl;
	u32 dma_write_engine_en;
	u32 dma_write_doorbell;
	u32 reserved2;
	u32 dma_write_channel_arb_weight_low;
	u32 dma_write_channel_arb_weight_high;
	u32 reserved3[3];
	u32 dma_read_engine_en;
	u32 dma_read_doorbell;
	u32 reserved4;
	u32 dma_read_channel_arb_weight_low;
	u32 dma_read_channel_arb_weight_high;
	u32 reserved5[3];
	u32 dma_write_int_status;
	u32 reserved6;
	u32 dma_write_int_mask;
	u32 dma_write_int_clear;
	u32 dma_write_err_status;
	u32 dma_write_done_imwr_low;
	u32 dma_write_done_imwr_high;
	u32 dma_write_abort_imwr_low;
	u32 dma_write_abort_imwr_high;
	u16 dma_write_ch_imwr_data[8];
	u32 reserved7[4];
	u32 dma_write_linked_list_err_en;
	u32 reserved8[3];
	u32 dma_read_int_status;
	u32 reserved9;
	u32 dma_read_int_mask;
	u32 dma_read_int_clear;
	u32 reserved10;
	u32 dma_read_err_status_low;
	u32 dma_read_err_status_high;
	u32 reserved11[2];
	u32 dma_read_linked_list_err_en;
	u32 reserved12;
	u32 dma_read_done_imwr_low;
	u32 dma_read_done_imwr_high;
	u32 dma_read_abort_imwr_low;
	u32 dma_read_abort_imwr_high;
	u16 dma_read_ch_imwr_data[8];
};

struct __packed pcie_dma_chan {
	u32 dma_ch_control1;
	u32 reserved1;
	u32 dma_transfer_size;
	u32 dma_sar_low;
	u32 dma_sar_high;
	u32 dma_dar_low;
	u32 dma_dar_high;
	u32 dma_llp_low;
	u32 dma_llp_high;
};

typedef enum mxlk_ep_engine_type {
	WRITE_ENGINE,
	READ_ENGINE
} mxlk_ep_engine_type;

static void __iomem *mxlk_ep_get_dma_addr(struct pci_epc *epc)
{
	struct dw_pcie_ep *ep = epc_get_drvdata(epc);
	struct dw_pcie *pci = to_dw_pcie_from_ep(ep);

	void __iomem *dma_base = pci->dbi_base + DMA_DBI_OFFSET;

	return dma_base;
}

static int mxlk_ep_dma_err_status(void __iomem *err_status, int chan)
{
	if (mxlk_rd32(err_status, 0) &
	    (DMA_AR_ERROR_CH_MASK(chan) | DMA_LL_ERROR_CH_MASK(chan)))
		return -EIO;

	return 0;
}

static int mxlk_ep_dma_rd_err_status_high(void __iomem *err_status, int chan)
{
	if (mxlk_rd32(err_status, 0) &
	    (DMA_UNREQ_ERROR_CH_MASK(chan) |
	     DMA_CPL_ABORT_ERROR_CH_MASK(chan) |
	     DMA_CPL_TIMEOUT_ERROR_CH_MASK(chan) |
	     DMA_DATA_POI_ERROR_CH_MASK(chan)))
		return -EIO;

	return 0;
}

int mxlk_ep_dma_write(struct pci_epc *epc, dma_addr_t dst, dma_addr_t src,
		      size_t len)
{
	int ret = 0;
	void __iomem *dma_base = mxlk_ep_get_dma_addr(epc);
	u32 src_low = (uint32_t)src;
	u32 dest_low = (uint32_t)dst;
#ifdef CONFIG_64BIT
	u32 src_high = (uint32_t)(src >> 32);
	u32 dest_high = (uint32_t)(dst >> 32);
#else
	u32 src_high = 0;
	u32 dest_high = 0;
#endif
	struct pcie_dma_chan *dma_chan =
		(struct pcie_dma_chan *)(dma_base + DMA_WRITE_CHAN0_OFFSET);
	struct pcie_dma_reg *dma_reg = (struct pcie_dma_reg *)dma_base;

	mxlk_wr32(&dma_chan->dma_transfer_size, 0, len);

	mxlk_wr32(&dma_chan->dma_sar_low, 0, src_low);
	mxlk_wr32(&dma_chan->dma_sar_high, 0, src_high);
	mxlk_wr32(&dma_chan->dma_dar_low, 0, dest_low);
	mxlk_wr32(&dma_chan->dma_dar_high, 0, dest_high);

	/* Start DMA transfer. */
	mxlk_wr32(&dma_reg->dma_write_doorbell, 0, DMA_DEF_CHANNEL);

	/* Wait for DMA transfer to complete. */
	while (!(mxlk_rd32(&dma_reg->dma_write_int_status, 0) &
		 (DMA_ABORT_INTERRUPT_CH_MASK(DMA_DEF_CHANNEL) |
		  DMA_DONE_INTERRUPT_CH_MASK(DMA_DEF_CHANNEL)))) {
	}

	ret = mxlk_ep_dma_err_status(&dma_reg->dma_write_err_status,
				     DMA_DEF_CHANNEL);

	/* Clear the done interrupt. */
	mxlk_wr32(&dma_reg->dma_write_int_clear, 0,
		  DMA_DONE_INTERRUPT_CH_MASK(DMA_DEF_CHANNEL));

	return ret;
}

int mxlk_ep_dma_read(struct pci_epc *epc, dma_addr_t dst, dma_addr_t src,
		     size_t len)
{
	int ret = 0;
	void __iomem *dma_base = mxlk_ep_get_dma_addr(epc);
	u32 src_low = (uint32_t)src;
	u32 dest_low = (uint32_t)dst;
#ifdef CONFIG_64BIT
	u32 src_high = (uint32_t)(src >> 32);
	u32 dest_high = (uint32_t)(dst >> 32);
#else
	u32 src_high = 0;
	u32 dest_high = 0;
#endif
	struct pcie_dma_chan *dma_chan =
		(struct pcie_dma_chan *)(dma_base + DMA_READ_CHAN0_OFFSET);
	struct pcie_dma_reg *dma_reg = (struct pcie_dma_reg *)dma_base;

	mxlk_wr32(&dma_chan->dma_transfer_size, 0, len);

	mxlk_wr32(&dma_chan->dma_sar_low, 0, src_low);
	mxlk_wr32(&dma_chan->dma_sar_high, 0, src_high);
	mxlk_wr32(&dma_chan->dma_dar_low, 0, dest_low);
	mxlk_wr32(&dma_chan->dma_dar_high, 0, dest_high);

	/* Start DMA transfer. */
	mxlk_wr32(&dma_reg->dma_read_doorbell, 0, DMA_DEF_CHANNEL);

	/* Wait for DMA transfer to complete. */
	while (!(mxlk_rd32(&dma_reg->dma_read_int_status, 0) &
		 (DMA_ABORT_INTERRUPT_CH_MASK(DMA_DEF_CHANNEL) |
		  DMA_DONE_INTERRUPT_CH_MASK(DMA_DEF_CHANNEL)))) {
	}

	if (mxlk_ep_dma_err_status(&dma_reg->dma_read_err_status_low,
				   DMA_DEF_CHANNEL))
		ret = -EIO;
	if (mxlk_ep_dma_rd_err_status_high(&dma_reg->dma_read_err_status_high,
					   DMA_DEF_CHANNEL))
		ret = -EIO;

	/* Clear the done interrupt. */
	mxlk_wr32(&dma_reg->dma_read_int_clear, 0,
		  DMA_DONE_INTERRUPT_CH_MASK(DMA_DEF_CHANNEL));

	return ret;
}

static void mxlk_ep_dma_disable(void __iomem *dma_base, mxlk_ep_engine_type rw)
{
	struct pcie_dma_reg *dma_reg = (struct pcie_dma_reg *)(dma_base);
	void __iomem *engine_en = (rw == WRITE_ENGINE) ?
					&dma_reg->dma_write_engine_en :
					&dma_reg->dma_read_engine_en;

	mxlk_wr32(engine_en, 0, 0);

	/* Wait until the engine is disabled. */
	while (mxlk_rd32(engine_en, 0) & DMA_ENGINE_EN_MASK) {
	}
}

static void mxlk_ep_dma_enable(void __iomem *dma_base, mxlk_ep_engine_type rw)
{
	struct pcie_dma_reg *dma_reg = (struct pcie_dma_reg *)(dma_base);
	void __iomem *engine_en = (rw == WRITE_ENGINE) ?
					&dma_reg->dma_write_engine_en :
					&dma_reg->dma_read_engine_en;
	void __iomem *int_mask = (rw == WRITE_ENGINE) ?
					&dma_reg->dma_write_int_mask:
					&dma_reg->dma_read_int_mask;
	void __iomem *int_clear = (rw == WRITE_ENGINE) ?
					&dma_reg->dma_write_int_clear:
					&dma_reg->dma_read_int_clear;

	u32 offset = (rw == WRITE_ENGINE) ? DMA_WRITE_CHAN0_OFFSET :
					    DMA_READ_CHAN0_OFFSET;
	struct pcie_dma_chan *dma_chan =
		(struct pcie_dma_chan *)(dma_base + offset);

	mxlk_wr32(engine_en, 0, DMA_ENGINE_EN_MASK);

	/* Mask all interrupts. */
	mxlk_wr32(int_mask, 0, DMA_ALL_INTERRUPT_MASK);

	/* Clear all interrupts. */
	mxlk_wr32(int_clear, 0, DMA_ALL_INTERRUPT_MASK);

	/* Enable local interrupt status (LIE). */
	mxlk_wr32(&dma_chan->dma_ch_control1, 0, DMA_CH_CONTROL1_LIE_MASK);
}

void mxlk_ep_dma_uninit(struct pci_epc *epc)
{
	void __iomem *dma_base = mxlk_ep_get_dma_addr(epc);

	mxlk_ep_dma_disable(dma_base, WRITE_ENGINE);
	mxlk_ep_dma_disable(dma_base, READ_ENGINE);
}

void mxlk_ep_dma_init(struct pci_epc *epc)
{
	void __iomem *dma_base = mxlk_ep_get_dma_addr(epc);

	/* Disable the DMA read/write engine. */
	mxlk_ep_dma_uninit(epc);

	mxlk_ep_dma_enable(dma_base, WRITE_ENGINE);
	mxlk_ep_dma_enable(dma_base, READ_ENGINE);
}
