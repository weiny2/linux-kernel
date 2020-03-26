// SPDX-License-Identifier: GPL-2.0-only
/*
 * KeemBay OCS Hash control unit Crypto Driver.
 *
 * Copyright (C) 2018-2019 Intel Corporation
 */


#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/slab.h>
#include <linux/err.h>
#include <linux/clk.h>
#include <linux/io.h>
#include <linux/hw_random.h>
#include <linux/platform_device.h>

#include <linux/device.h>
#include <linux/init.h>
#include <linux/errno.h>
#include <linux/interrupt.h>
#include <linux/irq.h>
#include <linux/scatterlist.h>
#include <linux/of_device.h>
#include <linux/delay.h>
#include <linux/crypto.h>
#include <linux/cryptohash.h>
#include <crypto/scatterwalk.h>
#include <crypto/algapi.h>
#include <crypto/sha.h>
#include <crypto/hash.h>
#include <crypto/internal/hash.h>

#include "thunderbay-ocs-hcu-core.h"
#include "ocs-hcu.h"

#define OCS_HCU_MODE 0x00
#define OCS_HCU_CHAIN 0x04
#define OCS_HCU_OPERATION 0x08
#define OCS_HCU_KEY_0 0x0C
#define OCS_HCU_KEY_1 0x10
#define OCS_HCU_KEY_2 0x14
#define OCS_HCU_KEY_3 0x18
#define OCS_HCU_KEY_4 0x1C
#define OCS_HCU_KEY_5 0x20
#define OCS_HCU_KEY_6 0x24
#define OCS_HCU_KEY_7 0x28
#define OCS_HCU_KEY_8 0x2C
#define OCS_HCU_KEY_9 0x30
#define OCS_HCU_KEY_10 0x34
#define OCS_HCU_KEY_11 0x38
#define OCS_HCU_KEY_12 0x3C
#define OCS_HCU_KEY_13 0x40
#define OCS_HCU_KEY_14 0x44
#define OCS_HCU_KEY_15 0x48
#define OCS_HCU_ISR 0x50
#define OCS_HCU_IER 0x54
#define OCS_HCU_STATUS 0x58
#define OCS_HCU_MSG_LEN_LO 0x60
#define OCS_HCU_MSG_LEN_HI 0x64
#define OCS_HCU_KEY_BYTE_ORDER_CFG 0x80
#define OCS_HCU_DMA_SRC_ADDR 0x400
#define OCS_HCU_DMA_DST_ADDR 0x404
#define OCS_HCU_DMA_SRC_SIZE 0x408
#define OCS_HCU_DMA_DST_SIZE 0x40C
#define OCS_HCU_DMA_DMA_MODE 0x410
#define OCS_HCU_DMA_OTHER_MODE 0x414
#define OCS_HCU_DMA_NEXT_SRC_DESCR 0x418
#define OCS_HCU_DMA_NEXT_DST_DESCR 0x41C
#define OCS_HCU_DMA_WHILE_ACTIVE_MODE 0x420
#define OCS_HCU_DMA_LOG 0x424
#define OCS_HCU_DMA_STATUS 0x428
#define OCS_HCU_DMA_PERF_CNTR 0x42C
#define OCS_HCU_DMA_VALID_SAI_31_0 0x440
#define OCS_HCU_DMA_VALID_SAI_63_32 0x444
#define OCS_HCU_DMA_VALID_SAI_95_64 0x448
#define OCS_HCU_DMA_VALID_SAI_127_96 0x44C
#define OCS_HCU_DMA_VALID_SAI_159_128 0x450
#define OCS_HCU_DMA_VALID_SAI_191_160 0x454
#define OCS_HCU_DMA_VALID_SAI_223_192 0x458
#define OCS_HCU_DMA_VALID_SAI_255_224 0x45C
#define OCS_HCU_DMA_MSI_ISR 0x480
#define OCS_HCU_DMA_MSI_IER 0x484
#define OCS_HCU_DMA_MSI_MASK 0x488
#define OCS_HCU_DMA_MSI_MA 0x800
#define OCS_HCU_DMA_MSI_DA 0x804
#define OCS_HCU_DMA_MSI_EN 0x808
#define OCS_HCU_DMA_INBUFFER_WRITE_FIFO 0x600
#define OCS_HCU_DMA_OUTBUFFER_READ_FIFO 0x700

/* Register bit definitions. */
#define HCU_STATUS_BUSY_MASK BIT(0)

#define HCU_IRQ_HASH_DONE BIT(2)
#define HCU_IRQ_HASH_ERR (BIT(3) | BIT(1) | BIT(0))

#define HCU_DMA_IRQ_SRC_DONE BIT(0)
#define HCU_DMA_IRQ_DST_DONE BIT(1)
#define HCU_DMA_IRQ_SAI_ERR BIT(2)
#define HCU_DMA_IRQ_BAD_COMP_ERR BIT(3)
#define HCU_DMA_IRQ_INBUF_RD_ERR BIT(4)
#define HCU_DMA_IRQ_INBUF_WD_ERR BIT(5)
#define HCU_DMA_IRQ_OUTBUF_WR_ERR BIT(6)
#define HCU_DMA_IRQ_OUTBUF_RD_ERR BIT(7)
#define HCU_DMA_IRQ_CRD_ERR BIT(8)
#define HCU_DMA_IRQ_ERR_MASK (HCU_DMA_IRQ_SAI_ERR | \
			HCU_DMA_IRQ_BAD_COMP_ERR | \
			HCU_DMA_IRQ_INBUF_RD_ERR | \
			HCU_DMA_IRQ_INBUF_WD_ERR | \
			HCU_DMA_IRQ_OUTBUF_WR_ERR | \
			HCU_DMA_IRQ_OUTBUF_RD_ERR | \
			HCU_DMA_IRQ_CRD_ERR)

#define HCU_DMA_SNOOP_MASK (0x7 << 28)
#define HCU_DMA_SRC_LL_EN BIT(25)
#define HCU_DMA_EN BIT(31)
#define HCU_DMA_DST_LL_EN BIT(24)

#define HCU_HMAC_OFFSET 20
#define HCU_ALGO_OFFSET 16
#define HCU_HMAC_MASK BIT(HCU_HMAC_OFFSET)

#define HCU_CHAIN_WRITE_ENDIANNESS_BIT 30
#define HCU_CHAIN_READ_ENDIANNESS_BIT 28
#define HCU_DATA_WRITE_ENDIANNESS_BIT 26

#define HCU_DMA_MSI_UNMASK BIT(0)
#define HCU_DMA_MSI_DISABLE 0
#define HCU_IRQ_DISABLE 0

#define OCS_HCU_NUM_CHAINS_SHA256_224_SM3 8
#define OCS_HCU_NUM_CHAINS_SHA384_512 16

#define OCS_HCU_START BIT(0)
#define OCS_HCU_TERMINATE BIT(1)

#define DEBUG

static inline void ocs_writel(u32 val, struct ocs_hcu_dev *hcu_dev, u32 offset)
{
#ifdef DEBUG
	dev_info(hcu_dev->dev, "Writing: 0x%08x to offset 0x%x.\n",
		val, offset);
#endif /* DEBUG */

	writel(val, hcu_dev->io_base + offset);
}

static inline u32 ocs_readl(struct ocs_hcu_dev *hcu_dev, u32 offset)
{
	u32 val = readl(hcu_dev->io_base + offset);

#ifdef DEBUG
	dev_info(hcu_dev->dev, "Reading: 0x%08x from offset 0x%x.\n",
		val, offset);
#endif /* DEBUG */
	return val;
}

static inline u32 ocs_hcu_num_chains(u32 algo)
{
	switch (algo) {
	case OCS_HCU_ALGO_SHA224:
	case OCS_HCU_ALGO_SHA256:
	case OCS_HCU_ALGO_SM3:
		return OCS_HCU_NUM_CHAINS_SHA256_224_SM3;
	case OCS_HCU_ALGO_SHA384:
	case OCS_HCU_ALGO_SHA512:
		return OCS_HCU_NUM_CHAINS_SHA384_512;
	default:
		return 0;
	};
}

void ocs_hcu_irq_en(struct ocs_hcu_dev *hcu_dev)
{
	ocs_writel(HCU_IRQ_HASH_DONE | HCU_IRQ_HASH_ERR, hcu_dev, OCS_HCU_IER);
	/* Only operating on DMA source completion and error interrupts. */
	ocs_writel(HCU_DMA_IRQ_ERR_MASK | HCU_DMA_IRQ_SRC_DONE,
		   hcu_dev, OCS_HCU_DMA_MSI_IER);
	/* Unmask */
	ocs_writel(HCU_DMA_MSI_UNMASK, hcu_dev, OCS_HCU_DMA_MSI_MASK);
}

void ocs_hcu_irq_dis(struct ocs_hcu_dev *hcu_dev)
{
	ocs_writel(HCU_IRQ_DISABLE, hcu_dev, OCS_HCU_IER);
	ocs_writel(HCU_DMA_MSI_DISABLE, hcu_dev, OCS_HCU_DMA_MSI_IER);
}

void ocs_hcu_get_intermediate_data(struct ocs_hcu_dev *hcu_dev,
				    struct ocs_hcu_idata_desc *data, u32 algo)
{
	int i;
	const int n = ocs_hcu_num_chains(algo);
	u32 *chain = (u32 *)data->digest;

	for (i = 0; i < n; i++)
		chain[i] = ocs_readl(hcu_dev, OCS_HCU_CHAIN);

	data->msg_len_lo = ocs_readl(hcu_dev, OCS_HCU_MSG_LEN_LO);
	data->msg_len_hi = ocs_readl(hcu_dev, OCS_HCU_MSG_LEN_HI);
}

void ocs_hcu_set_intermediate_data(struct ocs_hcu_dev *hcu_dev,
				   struct ocs_hcu_idata_desc *data, u32 algo)
{
	int i;
	const int n = ocs_hcu_num_chains(algo);
	u32 *chain = (u32 *)data->digest;

	for (i = 0; i < n; i++)
		ocs_writel(chain[i], hcu_dev, OCS_HCU_CHAIN);

	ocs_writel(data->msg_len_lo, hcu_dev, OCS_HCU_MSG_LEN_LO);
	ocs_writel(data->msg_len_hi, hcu_dev, OCS_HCU_MSG_LEN_HI);
}

void ocs_hcu_hw_init(struct ocs_hcu_dev *hcu_dev)
{
	u32 cfg = 0;

	if ((hcu_dev->flags & HCU_FLAGS_HCU_INIT) == 0) {
		/* Initialize hardware. */
		cfg = (KMB_HCU_ENDIANNESS_MASK <<
		       HCU_DATA_WRITE_ENDIANNESS_BIT);
		ocs_writel(cfg, hcu_dev, OCS_HCU_MODE);
		ocs_hcu_irq_en(hcu_dev);
		hcu_dev->flags |= HCU_FLAGS_HCU_INIT;
	}
}

/* To be called after ocs_hcu_hw_init */
int ocs_hcu_hw_cfg(struct ocs_hcu_dev *hcu_dev, u32 algo)
{
	u32 cfg = ocs_readl(hcu_dev, OCS_HCU_MODE);
	u32 ocs_algo = algo & OCS_HCU_ALGO_MASK;

	if (ocs_algo != OCS_HCU_ALGO_SHA256 &&
	    ocs_algo != OCS_HCU_ALGO_SHA224 &&
	    ocs_algo != OCS_HCU_ALGO_SHA384 &&
	    ocs_algo != OCS_HCU_ALGO_SHA512 &&
	    ocs_algo != OCS_HCU_ALGO_SM3)
		return -EINVAL;

	if ((hcu_dev->flags & HCU_FLAGS_HCU_INIT) == 0)
		return -EPERM;

	cfg |= ocs_algo << HCU_ALGO_OFFSET;
	cfg &= ~HCU_HMAC_MASK;
	cfg |= ((algo & OCS_HCU_ALGO_HMAC_MASK) >>
	       OCS_HCU_ALGO_HMAC_SHIFT) << HCU_HMAC_OFFSET;

	ocs_writel(cfg, hcu_dev, OCS_HCU_MODE);

	return 0;
}

void ocs_hcu_hw_disable(struct ocs_hcu_dev *hcu_dev)
{
	if ((hcu_dev->flags & HCU_FLAGS_HCU_INIT) == HCU_FLAGS_HCU_INIT) {
		/* Clear hardware. */
		ocs_writel(0, hcu_dev, OCS_HCU_MODE);
		ocs_hcu_irq_dis(hcu_dev);
		hcu_dev->flags &= ~HCU_FLAGS_HCU_INIT;
	}
}

void ocs_hcu_tx_data_done(struct ocs_hcu_dev *hcu_dev)
{
	u32 op = OCS_HCU_TERMINATE;

	ocs_writel(op, hcu_dev, OCS_HCU_OPERATION);
}

void ocs_hcu_ll_dma_start(struct ocs_hcu_dev *hcu_dev, dma_addr_t head,
			  bool terminate)
{
	//u32 cfg = HCU_DMA_SNOOP_MASK | HCU_DMA_SRC_LL_EN | HCU_DMA_EN;
	u32 cfg = HCU_DMA_SNOOP_MASK | HCU_DMA_SRC_LL_EN |
		HCU_DMA_EN | HCU_DMA_DST_LL_EN;
	u32 op = OCS_HCU_START;

	if (!head || head > OCS_HCU_DMA_MAX_ADDR_MASK)
		return;

	hcu_dev->flags |= HCU_FLAGS_HCU_ACTIVE;

	ocs_writel(head, hcu_dev, OCS_HCU_DMA_NEXT_DST_DESCR);

	ocs_writel(head, hcu_dev, OCS_HCU_DMA_NEXT_SRC_DESCR);
	ocs_writel(0, hcu_dev, OCS_HCU_DMA_SRC_SIZE);
	ocs_writel(0, hcu_dev, OCS_HCU_DMA_DST_SIZE);
	ocs_writel(op, hcu_dev, OCS_HCU_OPERATION);
	ocs_writel(cfg, hcu_dev, OCS_HCU_DMA_DMA_MODE);
	if (terminate)
		ocs_hcu_tx_data_done(hcu_dev);
}

void ocs_hcu_finish_req(struct ocs_hcu_dev *hcu_dev, u32 algo,
			struct ocs_hcu_idata_desc *data, int error)
{
	if (hcu_dev->flags & HCU_FLAGS_HCU_DONE && error == 0) {
		/* Get the digest and message length. */
		ocs_hcu_get_intermediate_data(hcu_dev, data, algo);
		/* Clear the HCU flags for the next request. */
		hcu_dev->flags &= ~(HCU_FLAGS_FINISH_REQ_MASK);
	} else if (error) {
		hcu_dev->flags |= HCU_FLAGS_HCU_OP_ERR;
	}
}

irqreturn_t ocs_hcu_irq_handler(int irq, void *dev_id)
{
	struct ocs_hcu_dev *hcu_dev = dev_id;
	irqreturn_t rc = IRQ_NONE;
	u32 hcu_irq = ocs_readl(hcu_dev, OCS_HCU_ISR);
	u32 dma_irq = ocs_readl(hcu_dev, OCS_HCU_DMA_MSI_ISR);

	/* Check the HCU status. */
	if (hcu_irq & HCU_IRQ_HASH_DONE) {
		hcu_dev->flags |= HCU_FLAGS_HCU_DONE;
		rc = IRQ_WAKE_THREAD;
	} else if (hcu_irq & HCU_IRQ_HASH_ERR) {
		hcu_dev->flags |= HCU_FLAGS_HCU_ERR;
		rc = IRQ_WAKE_THREAD;
	}
	/* Clear the HCU interrupt. */
	ocs_writel(hcu_irq, hcu_dev, OCS_HCU_ISR);

	/* Check the DMA status. */
	if (dma_irq & HCU_DMA_IRQ_SRC_DONE) {
		/* DMA is complete, indicate that the HCU is done this
		 * transaction.
		 */
		hcu_dev->flags |= HCU_FLAGS_HCU_DONE;
		rc = IRQ_WAKE_THREAD;
	} else if (dma_irq & HCU_DMA_IRQ_ERR_MASK) {
		hcu_dev->flags |= HCU_FLAGS_HCU_DMA_ERR;
		rc = IRQ_WAKE_THREAD;
	}
	/* Clear the HCU DMA interrupt. */
	ocs_writel(dma_irq, hcu_dev, OCS_HCU_DMA_MSI_ISR);


	return rc;
}

MODULE_LICENSE("GPL v2");
