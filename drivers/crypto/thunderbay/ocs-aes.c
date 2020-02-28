// SPDX-License-Identifier: GPL-2.0-only
/*
 * KeemBay OCS AES Crypto Driver.
 *
 * Copyright (C) 2018-2019 Intel Corporation
 */

#define pr_fmt(fmt) KBUILD_MODNAME ": " fmt

#include <linux/dma-mapping.h>
#include <linux/slab.h>
#include <linux/platform_device.h>
#include <linux/interrupt.h>
#include <linux/completion.h>
#include <linux/jiffies.h>
#include "ocs-aes.h"
#include "thunderbay-ocs-aes-core.h"

#define AES_COMMAND_OFFSET						0x0000
#define AES_KEY_0_OFFSET						0x0004
#define AES_KEY_1_OFFSET						0x0008
#define AES_KEY_2_OFFSET						0x000C
#define AES_KEY_3_OFFSET						0x0010
#define AES_KEY_4_OFFSET						0x0014
#define AES_KEY_5_OFFSET						0x0018
#define AES_KEY_6_OFFSET						0x001C
#define AES_KEY_7_OFFSET						0x0020
#define AES_IV_0_OFFSET							0x0024
#define AES_IV_1_OFFSET							0x0028
#define AES_IV_2_OFFSET							0x002C
#define AES_IV_3_OFFSET							0x0030
#define AES_ACTIVE_OFFSET						0x0034
#define AES_STATUS_OFFSET						0x0038
#define AES_KEY_SIZE_OFFSET						0x0044
#define AES_IER_OFFSET							0x0048
#define AES_ISR_OFFSET							0x005C
#define AES_MULTIPURPOSE1_0_OFFSET				0x0200
#define AES_MULTIPURPOSE1_1_OFFSET				0x0204
#define AES_MULTIPURPOSE1_2_OFFSET				0x0208
#define AES_MULTIPURPOSE1_3_OFFSET				0x020C
#define AES_MULTIPURPOSE2_0_OFFSET				0x0220
#define AES_MULTIPURPOSE2_1_OFFSET				0x0224
#define AES_MULTIPURPOSE2_2_OFFSET				0x0228
#define AES_MULTIPURPOSE2_3_OFFSET				0x022C
#define AES_BYTE_ORDER_CFG_OFFSET				0x02C0
#define AES_TLEN_OFFSET							0x0300
#define AES_T_MAC_0_OFFSET						0x0304
#define AES_T_MAC_1_OFFSET						0x0308
#define AES_T_MAC_2_OFFSET						0x030C
#define AES_T_MAC_3_OFFSET						0x0310
#define AES_PLEN_OFFSET							0x0314
#define AES_A_DMA_SRC_ADDR_OFFSET				0x0400
#define AES_A_DMA_DST_ADDR_OFFSET				0x0404
#define AES_A_DMA_SRC_SIZE_OFFSET				0x0408
#define AES_A_DMA_DST_SIZE_OFFSET				0x040C
#define AES_A_DMA_DMA_MODE_OFFSET				0x0410
#define AES_A_DMA_NEXT_SRC_DESCR_OFFSET			0x0418
#define AES_A_DMA_NEXT_DST_DESCR_OFFSET			0x041C
#define AES_A_DMA_WHILE_ACTIVE_MODE_OFFSET		0x0420
#define AES_A_DMA_LOG_OFFSET					0x0424
#define AES_A_DMA_STATUS_OFFSET					0x0428
#define AES_A_DMA_PERF_CNTR_OFFSET				0x042C
#define AES_A_DMA_MSI_ISR_OFFSET				0x0480
#define AES_A_DMA_MSI_IER_OFFSET				0x0484
#define AES_A_DMA_INBUFFER_WRITE_FIFO_OFFSET	0x0600
#define AES_A_DMA_OUTBUFFER_READ_FIFO_OFFSET	0x0700

#define AES_DISABLE_INT		0x00000000
#define AES_COMPLETE_INT	0x00000002

#define AES_MAX_TAG_SIZE_U32	(4)

/* During CCM decrypt, the OCS block needs to finish
 * processing the ciphertext before the tag is
 * written. For 128-bit mode this required delay
 * is 28 OCS clock cycles. For 256-bit mode it is
 * 36 OCS clock cycles.
 */
/* TODO: Above stated values work OK for single operations.
 * However there is an issue when doing multiple operations in rapid
 * succession. E.g. running
 * 'kcapi -x 2 -c "ccm(aes)"
 * -q 14b14fe5b317411392861638ec383ae40ba95fefe34255dc
 * -t 2ec067887114bc370281de6f00836ce4 -l 16
 * -n 76becd9d27ca8a026215f32712 -k 43c1142877d9f450e12d7b6db47a85ba
 * -a 6a59aacadd416e465264c15e1a1e9bfa084687492710f9bda832e2571e468224 -d 2'
 * will work for the first operation, but the AES_DONE interrupt is not always
 * fired for the second (or some subsequent) operation. Haven't found a way of
 * recovering.
 * Testing shows a value of 1000 is marginal, i.e. works OK majority of the time
 * but occasionally fails. Setting to 10000 for now to allow a margin of safety
 * 10000 OCS clock cycles (with OCS clock set to 250MHz) is 40us)
 */
#define CCM_DECRYPT_DELAY_CLK_COUNT	(10000UL)

#define OCS_DWORD_SWAP(value) \
	(((value & 0x000000ff) << 24) | ((value & 0x0000ff00) << 8) | \
	 ((value & 0xff000000) >> 24) | ((value & 0x00ff0000) >> 8))

enum aes_counter_mode {
	AES_CTR_M_NO_INC = 0,
	AES_CTR_M_32_INC = 1,
	AES_CTR_M_64_INC = 2,
	AES_CTR_M_128_INC = 3,
};

struct B0_formatting {
	union {
		uint8_t flag;
		struct {
			uint8_t q_coding : 3;
			uint8_t t_coding : 3;
			uint8_t Adata : 1;
			uint8_t Reserved : 1;
		} flags_st;
	} flag_u;
	uint8_t N_and_Q[15];
};

DECLARE_COMPLETION(g_aes_complete);

#if OCS_DEBUG_VERBOSE
#include <asm/io.h>
inline void OCS_IOWRITE32(const uint32_t val, __iomem void *dst_addr)
{
	pr_info("\t\twrt:p:v 0x%x=0x%x \r\n",
			(unsigned int)(dst_addr - kmb_ocs_aes_dev->base_reg
				+ 0x88818000),
			(unsigned int)val);
	iowrite32(val, dst_addr);
}

inline uint32_t OCS_IOREAD32(__iomem void *src_addr)
{
	uint32_t val = ioread32(src_addr);

	pr_info("\t\trd:p:v 0x%x=0x%x \r\n",
			(unsigned int)(src_addr - kmb_ocs_aes_dev->base_reg
				+ 0x88818000),
			(unsigned int)val);
	return val;
}

inline void OCS_IOWRITE8(const uint8_t val, __iomem void *dst_addr)
{
	pr_info("\t\twrt_8:p:v 0x%x=0x%x \r\n",
			(unsigned int)(dst_addr - kmb_ocs_aes_dev->base_reg
				+ 0x30008000),
			(unsigned int)val);
	iowrite8(val, dst_addr);
}
#else /* OCS_DEBUG_VERBOSE */

#define OCS_IOWRITE32(val, addr) iowrite32(val, addr)
#define OCS_IOREAD32(addr) ioread32(addr)
#define OCS_IOWRITE8(val, addr) iowrite8(val, addr)

#endif /* OCS_DEBUG_VERBOSE */

#define OCS_WRAPPER_READ_CONFIG 0
#define OCS_WRAPPER_WRITE_CONFIG 16
#define OCS_64BIT_ADDRESS_CONVERSION 32

#define OCS_WRAPPER_CONFIGURATION_REG_0_OFFSET (0x0000)
#define OCS_WRAPPER_STATUS_BIT_FUSE_ATTACK_0_OFFSET (0x0004)

void ocs_wrapper_init(uint64_t base_address)
{
 uint32_t base_val;
 base_val = base_address >> OCS_64BIT_ADDRESS_CONVERSION;
 OCS_IOWRITE32( ((base_val << OCS_WRAPPER_READ_CONFIG) | (base_val << OCS_WRAPPER_WRITE_CONFIG)), kmb_ocs_aes_dev->base_wrapper_reg + OCS_WRAPPER_CONFIGURATION_REG_0_OFFSET);
}

/* Disable AES IRQ. */
static void aes_irq_disable(void)
{
	unsigned long flags;

	/* disable interrupt */
	OCS_IOWRITE32(AES_DISABLE_INT, kmb_ocs_aes_dev->base_reg +
			AES_A_DMA_MSI_IER_OFFSET);
	OCS_IOWRITE32(AES_DISABLE_INT, kmb_ocs_aes_dev->base_reg +
			AES_IER_OFFSET);

	/* Clear any pending interrupts */
	OCS_IOWRITE32(OCS_IOREAD32(kmb_ocs_aes_dev->base_reg +
				AES_A_DMA_MSI_ISR_OFFSET),
			kmb_ocs_aes_dev->base_reg + AES_A_DMA_MSI_ISR_OFFSET);

	OCS_IOWRITE32(OCS_IOREAD32(kmb_ocs_aes_dev->base_reg + AES_ISR_OFFSET),
			kmb_ocs_aes_dev->base_reg + AES_ISR_OFFSET);

	spin_lock_irqsave(&kmb_ocs_aes_dev->irq_lock, flags);
	if (kmb_ocs_aes_dev->irq_enabled) {
		disable_irq_nosync(kmb_ocs_aes_dev->irq);
		kmb_ocs_aes_dev->irq_enabled = false;
	}
	spin_unlock_irqrestore(&kmb_ocs_aes_dev->irq_lock, flags);
	//pr_info("AES IRQ disabled\n");    //TODO: remove
}

/* Enable AES IRQ. */
static void aes_irq_enable(void)
{
	unsigned long flags;

	reinit_completion(&g_aes_complete);

	spin_lock_irqsave(&kmb_ocs_aes_dev->irq_lock, flags);
	if (!kmb_ocs_aes_dev->irq_enabled) {
		enable_irq(kmb_ocs_aes_dev->irq);
		kmb_ocs_aes_dev->irq_enabled = true;
	}
	spin_unlock_irqrestore(&kmb_ocs_aes_dev->irq_lock, flags);

	/* Clear any pending interrupts */
	OCS_IOWRITE32(OCS_IOREAD32(kmb_ocs_aes_dev->base_reg +
				AES_A_DMA_MSI_ISR_OFFSET),
			kmb_ocs_aes_dev->base_reg + AES_A_DMA_MSI_ISR_OFFSET);

	OCS_IOWRITE32(OCS_IOREAD32(kmb_ocs_aes_dev->base_reg + AES_ISR_OFFSET),
			kmb_ocs_aes_dev->base_reg + AES_ISR_OFFSET);

	/* Enable interrupt
	 * default 32'b0
	 * 32'b0000_0000_0000_0000_0000_0000_0000_0010
	 * bits [31:3] - reserved
	 * bit [2] - EN_SKS_ERR
	 * bit [1] - EN_AES_COMPLETE
	 * bit [0] - reserved
	 */
	OCS_IOWRITE32(AES_COMPLETE_INT, kmb_ocs_aes_dev->base_reg +
			AES_IER_OFFSET);

	//TODO: what to set this to?
	OCS_IOWRITE32(0x00, kmb_ocs_aes_dev->base_reg +
			AES_A_DMA_MSI_IER_OFFSET);
}

irqreturn_t ocs_aes_irq_handler(int irq, void *dev_id)
{
	struct keembay_ocs_aes_dev *ocs_aes_dev = dev_id;
	uint32_t status_aes, status_dma;

	status_aes = OCS_IOREAD32(ocs_aes_dev->base_reg + AES_ISR_OFFSET);
	status_dma = OCS_IOREAD32(ocs_aes_dev->base_reg +
			AES_A_DMA_MSI_ISR_OFFSET);

	if (status_aes & AES_COMPLETE_INT)
		complete(&g_aes_complete);

	//TODO: check DMA interrupt?

	/* Clear interrupt */
	OCS_IOWRITE32(status_aes, ocs_aes_dev->base_reg + AES_ISR_OFFSET);
	OCS_IOWRITE32(status_dma, ocs_aes_dev->base_reg +
			AES_A_DMA_MSI_ISR_OFFSET);

	return IRQ_HANDLED;
}


int ocs_aes_set_key(const uint32_t key_size, const uint8_t *key)
{
	if (key_size != 16 && key_size != 32) {
		pr_err("AES key size %d not supported\n", key_size*8);
		return 1;	//OCS_INVALID_PARAM
	}

	if (key == NULL)
		return 1;	//OCS_INVALID_PARAM

	OCS_IOWRITE32(((uint32_t *)key)[0], kmb_ocs_aes_dev->base_reg +
			AES_KEY_0_OFFSET);
	OCS_IOWRITE32(((uint32_t *)key)[1], kmb_ocs_aes_dev->base_reg +
			AES_KEY_1_OFFSET);
	OCS_IOWRITE32(((uint32_t *)key)[2], kmb_ocs_aes_dev->base_reg +
			AES_KEY_2_OFFSET);
	OCS_IOWRITE32(((uint32_t *)key)[3], kmb_ocs_aes_dev->base_reg +
			AES_KEY_3_OFFSET);
	if (key_size == 32) {
		OCS_IOWRITE32(((uint32_t *)key)[4], kmb_ocs_aes_dev->base_reg +
				AES_KEY_4_OFFSET);
		OCS_IOWRITE32(((uint32_t *)key)[5], kmb_ocs_aes_dev->base_reg +
				AES_KEY_5_OFFSET);
		OCS_IOWRITE32(((uint32_t *)key)[6], kmb_ocs_aes_dev->base_reg +
				AES_KEY_6_OFFSET);
		OCS_IOWRITE32(((uint32_t *)key)[7], kmb_ocs_aes_dev->base_reg +
				AES_KEY_7_OFFSET);

		/* Write key size
		 * bits [31:1] - reserved
		 * bit [0] - AES_KEY_SIZE
		 *  0 - 128 bit key
		 *  1 - 256 bit key
		 */
		OCS_IOWRITE32(0x00000001, kmb_ocs_aes_dev->base_reg +
				AES_KEY_SIZE_OFFSET);
	} else {
		OCS_IOWRITE32(0x00000000, kmb_ocs_aes_dev->base_reg +
				AES_KEY_SIZE_OFFSET);
	}

	return 0;   //OCS_SUCCESS
}

//src_descriptor and dst_descriptor must be 8-byte aligned
//and in lower 2GB of RAM (i.e. 32-bit address sufficient)
int ocs_aes_op(const dma_addr_t src_descriptor, const uint32_t src_size,
		const uint8_t *iv, const uint32_t iv_size,
		const enum aes_mode mode,
		const enum aes_instruction instruction,
		const dma_addr_t dst_descriptor)
{
	uint32_t val;

	//TODO: validate inputs
	//if last pt block is not 16 bytes for ECB or CBC return error
	//for CTS ensure >= 16bytes in total, else return error
	//if present IV must be 16 bytes (must be present for CBC, CTS
	//and CTR)??

	/* 32'b0000_0000_0000_0000_0000_0111_1111_1111
	 * default 0x00000000
	 * bit [10] - KEY_HI_LO_SWAP
	 * bit [9] - KEY_HI_SWAP_DWORDS_IN_OCTWORD
	 * bit [8] - KEY_HI_SWAP_BYTES_IN_DWORD
	 * bit [7] - KEY_LO_SWAP_DWORDS_IN_OCTWORD
	 * bit [6] - KEY_LO_SWAP_BYTES_IN_DWORD
	 * bit [5] - IV_SWAP_DWORDS_IN_OCTWORD
	 * bit [4] - IV_SWAP_BYTES_IN_DWORD
	 * bit [3] - DOUT_SWAP_DWORDS_IN_OCTWORD
	 * bit [2] - DOUT_SWAP_BYTES_IN_DWORD
	 * bit [1] - DOUT_SWAP_DWORDS_IN_OCTWORD
	 * bit [0] - DOUT_SWAP_BYTES_IN_DWORD
	 */
	OCS_IOWRITE32(0x000007FF, kmb_ocs_aes_dev->base_reg +
			AES_BYTE_ORDER_CFG_OFFSET);

	/* Write AES_COMMAND
	 * CIPHER_SELECT=AES, OPERATION_MODE=ECB, AES_INSTRUCTION=encrypt
	 * 32'b0000_0000_0000_0000_0000_0000_0000_0000
	 * bit [14] - CIPHER_SELECT - 0 AES
	 * bits [11:8] - AES_MODE
	 * bits [7:6] - AES_INSTRUCTION
	 * bits [3:2] - CTR_M_BITS
	 */
	val = (mode<<8) + (instruction<<6) + (AES_CTR_M_128_INC<<2);
	OCS_IOWRITE32(val, kmb_ocs_aes_dev->base_reg + AES_COMMAND_OFFSET);

	if (mode == AES_MODE_CTS) {
		/* Write length of last full data block */
		if (src_size == 0)
			val = 0;
		else {
			val = src_size % AES_BLOCK_SIZE;
			if (val == 0)
				val = AES_BLOCK_SIZE;
		}
		OCS_IOWRITE32(val, kmb_ocs_aes_dev->base_reg +
				AES_PLEN_OFFSET);
	}

	if (iv && (iv_size == 16)) {
		OCS_IOWRITE32(((uint32_t *)iv)[0], kmb_ocs_aes_dev->base_reg +
				AES_IV_0_OFFSET);
		OCS_IOWRITE32(((uint32_t *)iv)[1], kmb_ocs_aes_dev->base_reg +
				AES_IV_1_OFFSET);
		OCS_IOWRITE32(((uint32_t *)iv)[2], kmb_ocs_aes_dev->base_reg +
				AES_IV_2_OFFSET);
		OCS_IOWRITE32(((uint32_t *)iv)[3], kmb_ocs_aes_dev->base_reg +
				AES_IV_3_OFFSET);
	}

	aes_irq_enable();

	/* Write trigger bit
	 * default 32'b0
	 * bit [0] - assert to start
	 */
	OCS_IOWRITE32(0x00000001, kmb_ocs_aes_dev->base_reg +
			AES_ACTIVE_OFFSET);

	OCS_IOWRITE32(src_descriptor, kmb_ocs_aes_dev->base_reg +
			AES_A_DMA_NEXT_SRC_DESCR_OFFSET);
	OCS_IOWRITE32(0, kmb_ocs_aes_dev->base_reg +
			AES_A_DMA_SRC_SIZE_OFFSET);
	OCS_IOWRITE32(dst_descriptor, kmb_ocs_aes_dev->base_reg +
			AES_A_DMA_NEXT_DST_DESCR_OFFSET);
	OCS_IOWRITE32(0, kmb_ocs_aes_dev->base_reg + AES_A_DMA_DST_SIZE_OFFSET);


	/* DMA mode
	 * bit[31] - ACTIVE
	 * bit[25] - SRC_LINK_LIST_EN
	 * bit[24] - DST_LINK_LIST_EN
	 */
	OCS_IOWRITE32(0x83000000, kmb_ocs_aes_dev->base_reg +
			AES_A_DMA_DMA_MODE_OFFSET);

	if (mode == AES_MODE_CTS) {
		/* set LAST_GCX bit
		 * Write trigger bit
		 * default 32'b0
		 * bit [8] - LAST_GCX
		 */
		OCS_IOWRITE32(0x00000100, kmb_ocs_aes_dev->base_reg +
				AES_ACTIVE_OFFSET);
	} else {
		/* Write termination bit
		 * default 32'b0
		 * bit [1] - assert to indicate end of data
		 */
		OCS_IOWRITE32(0x00000002, kmb_ocs_aes_dev->base_reg +
				AES_ACTIVE_OFFSET);
	}


	//pr_info("###   Waiting for AES complete   ###\n"); //TODO: remove
	wait_for_completion(&g_aes_complete);
	reinit_completion(&g_aes_complete);
	//pr_info("###   AES complete   ###\n");	//TODO: remove
	//pr_info("DMA Status: 0x%08x\n", OCS_IOREAD32(
	//			kmb_ocs_aes_dev->base_reg +
	//			AES_A_DMA_STATUS_OFFSET));	//TODO: remove
	//pr_info("DMA Mode: 0x%08x\n", OCS_IOREAD32(kmb_ocs_aes_dev->base_reg +
	//			AES_A_DMA_DMA_MODE_OFFSET));	//TODO: remove

	aes_irq_disable();

	if (mode == AES_MODE_CTR) {
		((uint32_t *)iv)[0] = OCS_IOREAD32(kmb_ocs_aes_dev->base_reg +
			AES_IV_0_OFFSET);
		((uint32_t *)iv)[1] = OCS_IOREAD32(kmb_ocs_aes_dev->base_reg +
			AES_IV_1_OFFSET);
		((uint32_t *)iv)[2] = OCS_IOREAD32(kmb_ocs_aes_dev->base_reg +
			AES_IV_2_OFFSET);
		((uint32_t *)iv)[3] = OCS_IOREAD32(kmb_ocs_aes_dev->base_reg +
			AES_IV_3_OFFSET);
	}

	return 0;
}



static void gcm_construct_j0(const uint8_t *iv,
		const uint32_t iv_size, uint32_t *j0)
{
	if (iv_size == 12) {
		memcpy(j0, iv, iv_size);
		j0[0] = OCS_DWORD_SWAP(j0[0]);
		j0[1] = OCS_DWORD_SWAP(j0[1]);
		j0[2] = OCS_DWORD_SWAP(j0[2]);
		j0[3] = 0x00000001;
	} else {
		//TODO: needs GHASH function...
	}
}

int ocs_aes_gcm_op(const dma_addr_t src_descriptor, const uint32_t src_size,
		const uint8_t *iv, const uint32_t iv_size,
		const dma_addr_t aad_descriptor, const uint32_t aad_size,
		const uint32_t tag_size, const enum aes_instruction instruction,
		const dma_addr_t dst_descriptor, uint8_t *tag)
{
	uint32_t J0[4], val, tag_u32[AES_MAX_TAG_SIZE_U32];

	//TODO: check validity of arguments

	/* 32'b0000_0000_0000_0000_0000_0111_1111_1111
	 * default 0x00000000
	 * bit [10] - KEY_HI_LO_SWAP
	 * bit [9] - KEY_HI_SWAP_DWORDS_IN_OCTWORD
	 * bit [8] - KEY_HI_SWAP_BYTES_IN_DWORD
	 * bit [7] - KEY_LO_SWAP_DWORDS_IN_OCTWORD
	 * bit [6] - KEY_LO_SWAP_BYTES_IN_DWORD
	 * bit [5] - IV_SWAP_DWORDS_IN_OCTWORD
	 * bit [4] - IV_SWAP_BYTES_IN_DWORD
	 * bit [3] - DOUT_SWAP_DWORDS_IN_OCTWORD
	 * bit [2] - DOUT_SWAP_BYTES_IN_DWORD
	 * bit [1] - DOUT_SWAP_DWORDS_IN_OCTWORD
	 * bit [0] - DOUT_SWAP_BYTES_IN_DWORD
	 */
	OCS_IOWRITE32(0x000007FF, kmb_ocs_aes_dev->base_reg +
			AES_BYTE_ORDER_CFG_OFFSET);

	/* Write AES_COMMAND
	 * CIPHER_SELECT=AES, OPERATION_MODE=ECB, AES_INSTRUCTION=encrypt
	 * 32'b0000_0000_0000_0000_0000_0000_0000_0000
	 * bit [14] - CIPHER_SELECT - 0 AES
	 * bits [11:8] - AES_MODE
	 * bits [7:6] - AES_INSTRUCTION
	 * bits [3:2] - CTR_M_BITS
	 */
	val = (AES_MODE_GCM<<8) + (instruction<<6) + (AES_CTR_M_128_INC<<2);
	OCS_IOWRITE32(val, kmb_ocs_aes_dev->base_reg + AES_COMMAND_OFFSET);


	gcm_construct_j0(iv, iv_size, J0);
	OCS_IOWRITE32(J0[3], kmb_ocs_aes_dev->base_reg + AES_IV_0_OFFSET);
	OCS_IOWRITE32(J0[2], kmb_ocs_aes_dev->base_reg + AES_IV_1_OFFSET);
	OCS_IOWRITE32(J0[1], kmb_ocs_aes_dev->base_reg + AES_IV_2_OFFSET);
	OCS_IOWRITE32(J0[0], kmb_ocs_aes_dev->base_reg + AES_IV_3_OFFSET);

	/* Write tag length */
	OCS_IOWRITE32(tag_size, kmb_ocs_aes_dev->base_reg + AES_TLEN_OFFSET);

	/* Write length of last full data block */
	if (src_size == 0)
		val = 0;
	else {
		val = src_size % AES_BLOCK_SIZE;
		if (val == 0)
			val = AES_BLOCK_SIZE;
	}
	OCS_IOWRITE32(val, kmb_ocs_aes_dev->base_reg +
			AES_PLEN_OFFSET);

	/* write ciphertext length */
	val = src_size * 8;
	OCS_IOWRITE32(val, kmb_ocs_aes_dev->base_reg +
			AES_MULTIPURPOSE2_0_OFFSET);

	val = (uint32_t)((((uint64_t)(src_size)) * 8) >> 32);
	OCS_IOWRITE32(val, kmb_ocs_aes_dev->base_reg +
			AES_MULTIPURPOSE2_1_OFFSET);

	/* write aad length */
	val = aad_size * 8;
	OCS_IOWRITE32(val, kmb_ocs_aes_dev->base_reg +
			AES_MULTIPURPOSE2_2_OFFSET);

	val = (uint32_t)((((uint64_t)(aad_size)) * 8) >> 32);
	OCS_IOWRITE32(val, kmb_ocs_aes_dev->base_reg +
			AES_MULTIPURPOSE2_3_OFFSET);

	/* Write trigger bit
	 * default 32'b0
	 * bit [0] - assert to start
	 */
	OCS_IOWRITE32(0x00000001, kmb_ocs_aes_dev->base_reg +
			AES_ACTIVE_OFFSET);

	aes_irq_enable();

	if (aad_size) {
		/* DMA: Fetch AAD */
		OCS_IOWRITE32(aad_descriptor, kmb_ocs_aes_dev->base_reg +
				AES_A_DMA_NEXT_SRC_DESCR_OFFSET);
		OCS_IOWRITE32(aad_descriptor, kmb_ocs_aes_dev->base_reg +
				AES_A_DMA_NEXT_DST_DESCR_OFFSET);
		OCS_IOWRITE32(0, kmb_ocs_aes_dev->base_reg +
				AES_A_DMA_SRC_SIZE_OFFSET);
		/* DMA mode
		 * bit[31] - ACTIVE
		 * bit[25] - SRC_LINK_LIST_EN
		 */
		OCS_IOWRITE32(0x83000000, kmb_ocs_aes_dev->base_reg +
				AES_A_DMA_DMA_MODE_OFFSET);

		/* set LAST_GCX and LAST_ADATA bit
		 * (only after last batch of AAD)
		 * Write trigger bit
		 * default 32'b0
		 * bit [9] - LAST_ADATA
		 * bit [8] - LAST_GCX
		 */
		OCS_IOWRITE32(0x00000300, kmb_ocs_aes_dev->base_reg +
				AES_ACTIVE_OFFSET);


		/* Poll DMA_MSI_ISR SRC_DONE_INT_STATUS */
		//TODO: replace with interrupt?
		do {
			val = OCS_IOREAD32(kmb_ocs_aes_dev->base_reg +
					AES_A_DMA_MSI_ISR_OFFSET);
		} while ((val & 0x00000001) == 0);

	} else {
		/* Since no aad the LAST_GCX bit can be set now */

		/* set LAST_GCX and LAST_ADATA bit
		 * (only after last batch of AAD)
		 * Write trigger bit
		 * default 32'b0
		 * bit [9] - LAST_ADATA
		 * bit [8] - LAST_GCX
		 */
		OCS_IOWRITE32(0x00000300, kmb_ocs_aes_dev->base_reg +
				AES_ACTIVE_OFFSET);
	}

	/* Poll AES_ACTIVE LAST_GCX */
	do {
		val = OCS_IOREAD32(kmb_ocs_aes_dev->base_reg +
				AES_ACTIVE_OFFSET);
	} while (val & 0x00000100);

	OCS_IOWRITE32(0, kmb_ocs_aes_dev->base_reg + AES_A_DMA_SRC_SIZE_OFFSET);
	OCS_IOWRITE32(0, kmb_ocs_aes_dev->base_reg + AES_A_DMA_DST_SIZE_OFFSET);
	if (src_size) {
		/* Fetch plain text */
		OCS_IOWRITE32(src_descriptor, kmb_ocs_aes_dev->base_reg +
				AES_A_DMA_NEXT_SRC_DESCR_OFFSET);
		/* Set output address */
		OCS_IOWRITE32(dst_descriptor, kmb_ocs_aes_dev->base_reg +
				AES_A_DMA_NEXT_DST_DESCR_OFFSET);
		/* DMA mode
		 * bit[31] - ACTIVE
		 * bit[25] - SRC_LINK_LIST_EN
		 * bit[24] - DST_LINK_LIST_EN
		 */
		OCS_IOWRITE32(0x83000000, kmb_ocs_aes_dev->base_reg +
				AES_A_DMA_DMA_MODE_OFFSET);
	} else {
		OCS_IOWRITE32(0x80000000, kmb_ocs_aes_dev->base_reg +
				AES_A_DMA_DMA_MODE_OFFSET);
	}

	/* set LAST_GCX bit
	 * Write trigger bit
	 * default 32'b0
	 * bit [8] - LAST_GCX
	 */
	OCS_IOWRITE32(0x00000100, kmb_ocs_aes_dev->base_reg +
			AES_ACTIVE_OFFSET);

	//pr_info("DEBUG: Waiting for AES complete\n");	//TODO: remove
	wait_for_completion(&g_aes_complete);
	reinit_completion(&g_aes_complete);
	//pr_info("DEBUG: AES complete\n");	//TODO: remove
	//pr_info("DEBUG: DMA Status: 0x%08x\n",
	//		OCS_IOREAD32(kmb_ocs_aes_dev->base_reg +
	//			AES_A_DMA_STATUS_OFFSET));
	//pr_info("DEBUG: DMA Mode: 0x%08x\n",
	//		OCS_IOREAD32(kmb_ocs_aes_dev->base_reg +
	//			AES_A_DMA_DMA_MODE_OFFSET));

	tag_u32[0] = OCS_IOREAD32(kmb_ocs_aes_dev->base_reg +
			AES_T_MAC_3_OFFSET);
	tag_u32[1] = OCS_IOREAD32(kmb_ocs_aes_dev->base_reg +
			AES_T_MAC_2_OFFSET);
	tag_u32[2] = OCS_IOREAD32(kmb_ocs_aes_dev->base_reg +
			AES_T_MAC_1_OFFSET);
	tag_u32[3] = OCS_IOREAD32(kmb_ocs_aes_dev->base_reg +
			AES_T_MAC_0_OFFSET);

	/* Not swapping during initial assignment as macro would translate
	 * to 4 reg reads each
	 */
	tag_u32[0] = OCS_DWORD_SWAP(tag_u32[0]);
	tag_u32[1] = OCS_DWORD_SWAP(tag_u32[1]);
	tag_u32[2] = OCS_DWORD_SWAP(tag_u32[2]);
	tag_u32[3] = OCS_DWORD_SWAP(tag_u32[3]);


	if (tag)
		memcpy(tag, tag_u32, tag_size);

	aes_irq_disable();

	return 0;
}

static inline int ccm_compare_tag_to_yr(const uint8_t tag_size_bytes)
{
	uint8_t i = 0;
	uint32_t tag_u32[AES_MAX_TAG_SIZE_U32] = {0};
	uint32_t yr_u32[AES_MAX_TAG_SIZE_U32] = {0};
	const uint8_t *const tag_u8 = (const uint8_t *const)tag_u32;
	const uint8_t *const yr_u8 = (const uint8_t *const)yr_u32;

	if (tag_size_bytes > (AES_MAX_TAG_SIZE_U32 * 4))
		return 1;	/* TODO: revist this error code */

	/* read Tag */
	tag_u32[0] = OCS_IOREAD32(kmb_ocs_aes_dev->base_reg +
					AES_T_MAC_0_OFFSET);
	tag_u32[1] = OCS_IOREAD32(kmb_ocs_aes_dev->base_reg +
					AES_T_MAC_1_OFFSET);
	tag_u32[2] = OCS_IOREAD32(kmb_ocs_aes_dev->base_reg +
					AES_T_MAC_2_OFFSET);
	tag_u32[3] = OCS_IOREAD32(kmb_ocs_aes_dev->base_reg +
					AES_T_MAC_3_OFFSET);

	/* read Yr */
	yr_u32[0] = OCS_IOREAD32(kmb_ocs_aes_dev->base_reg +
					AES_MULTIPURPOSE2_0_OFFSET);
	yr_u32[1] = OCS_IOREAD32(kmb_ocs_aes_dev->base_reg +
					AES_MULTIPURPOSE2_1_OFFSET);
	yr_u32[2] = OCS_IOREAD32(kmb_ocs_aes_dev->base_reg +
					AES_MULTIPURPOSE2_2_OFFSET);
	yr_u32[3] = OCS_IOREAD32(kmb_ocs_aes_dev->base_reg +
					AES_MULTIPURPOSE2_3_OFFSET);

	/* compare Tag and Yr */
	for (i = 0; i < tag_size_bytes; i++) {
		if (tag_u8[i] != yr_u8[i])
			return 1;
	}
	return 0;
}

/* TODO: verify the validity of this wait */
static inline void ccm_hw_cycles_delay(uint32_t ocs_clk_cycles)
{
	/* reset PERF_CNTR to 0 */
	OCS_IOWRITE32(0x00000000, kmb_ocs_aes_dev->base_reg +
			AES_A_DMA_PERF_CNTR_OFFSET);

	/* activate PERF_CNTR */
	OCS_IOWRITE32(0x00000001, kmb_ocs_aes_dev->base_reg +
			AES_A_DMA_WHILE_ACTIVE_MODE_OFFSET);

	while (OCS_IOREAD32(kmb_ocs_aes_dev->base_reg +
				AES_A_DMA_PERF_CNTR_OFFSET) < ocs_clk_cycles)
		;

	/* deactive PERF_CNTR */
	OCS_IOWRITE32(0x00000000, kmb_ocs_aes_dev->base_reg +
			AES_A_DMA_WHILE_ACTIVE_MODE_OFFSET);
}

int ocs_aes_ccm_op(const dma_addr_t src_descriptor, const uint32_t src_size,
		const uint8_t *iv,
		const dma_addr_t aad_descriptor, const uint32_t aad_size,
		const uint8_t *in_tag, const uint32_t tag_size,
		const enum aes_instruction instruction,
		const dma_addr_t dst_descriptor, uint8_t *auth_res)
{
	uint32_t val, i;
	uint32_t *ctr_ptr = NULL;
	struct B0_formatting B0;

	//TODO: check validity of arguments

	/* 32'b0000_0000_0000_0000_0000_0111_1111_1111
	 * default 0x00000000
	 * bit [10] - KEY_HI_LO_SWAP
	 * bit [9] - KEY_HI_SWAP_DWORDS_IN_OCTWORD
	 * bit [8] - KEY_HI_SWAP_BYTES_IN_DWORD
	 * bit [7] - KEY_LO_SWAP_DWORDS_IN_OCTWORD
	 * bit [6] - KEY_LO_SWAP_BYTES_IN_DWORD
	 * bit [5] - IV_SWAP_DWORDS_IN_OCTWORD
	 * bit [4] - IV_SWAP_BYTES_IN_DWORD
	 * bit [3] - DOUT_SWAP_DWORDS_IN_OCTWORD
	 * bit [2] - DOUT_SWAP_BYTES_IN_DWORD
	 * bit [1] - DOUT_SWAP_DWORDS_IN_OCTWORD
	 * bit [0] - DOUT_SWAP_BYTES_IN_DWORD
	 */
	OCS_IOWRITE32(0x000007FF, kmb_ocs_aes_dev->base_reg +
			AES_BYTE_ORDER_CFG_OFFSET);

	/* Write AES_COMMAND
	 * 32'b0000_0000_0000_0000_0000_0000_0000_0000
	 * bit [14] - CIPHER_SELECT - 0 AES
	 * bits [11:8] - AES_MODE
	 * bits [7:6] - AES_INSTRUCTION
	 * bits [3:2] - CTR_M_BITS
	 */
	val = (AES_MODE_CCM<<8) + (instruction<<6) + (AES_CTR_M_128_INC<<2);
	OCS_IOWRITE32(val, kmb_ocs_aes_dev->base_reg + AES_COMMAND_OFFSET);

	/* nonce is already converted to ctr0 before
	 * being passed into this function (as iv)
	 */
	ctr_ptr = (uint32_t *)&iv[0];
	OCS_IOWRITE32(OCS_DWORD_SWAP(*ctr_ptr), kmb_ocs_aes_dev->base_reg +
			AES_MULTIPURPOSE1_3_OFFSET);
	ctr_ptr = (uint32_t *)&iv[4];
	OCS_IOWRITE32(OCS_DWORD_SWAP(*ctr_ptr), kmb_ocs_aes_dev->base_reg +
			AES_MULTIPURPOSE1_2_OFFSET);
	ctr_ptr = (uint32_t *)&iv[8];
	OCS_IOWRITE32(OCS_DWORD_SWAP(*ctr_ptr), kmb_ocs_aes_dev->base_reg +
			AES_MULTIPURPOSE1_1_OFFSET);
	ctr_ptr = (uint32_t *)&iv[12];
	OCS_IOWRITE32(OCS_DWORD_SWAP(*ctr_ptr), kmb_ocs_aes_dev->base_reg +
			AES_MULTIPURPOSE1_0_OFFSET);

	/* Write tag length */
	OCS_IOWRITE32(tag_size, kmb_ocs_aes_dev->base_reg + AES_TLEN_OFFSET);

	/* Write length of last full data block */
	if (src_size == 0)
		val = 0;
	else {
		val = src_size % AES_BLOCK_SIZE;
		if (val == 0)
			val = AES_BLOCK_SIZE;
	}
	OCS_IOWRITE32(val, kmb_ocs_aes_dev->base_reg +
			AES_PLEN_OFFSET);

	/* Write trigger bit
	 * default 32'b0
	 * bit [0] - assert to start
	 */
	OCS_IOWRITE32(0x00000001, kmb_ocs_aes_dev->base_reg +
			AES_ACTIVE_OFFSET);

	aes_irq_enable();

	/* Write B0 */
	memset(&B0, 0, 16);
	B0.flag_u.flags_st.Adata = (aad_size == 0) ? 0 : 1;
	B0.flag_u.flags_st.t_coding = (uint8_t)(((tag_size-2)/2) & 0x07);
	B0.flag_u.flags_st.q_coding = iv[0];
	for (i = 0; i < (AES_BLOCK_SIZE - 2 - iv[0]); i++)
		B0.N_and_Q[i] = iv[i + 1];
	/* Assigning size value from dword(val) to byte array till val
	 * becomes zero. Start at end of array and work back.
	 */
	val = src_size;
	i = sizeof(B0.N_and_Q) - 1;
	while (val) {
		B0.N_and_Q[i] = val & 0xff;
		val = val >> 8; // Divide by 256
		i--;
	}

	OCS_IOWRITE8(B0.flag_u.flag, kmb_ocs_aes_dev->base_reg +
			AES_A_DMA_INBUFFER_WRITE_FIFO_OFFSET);
	OCS_IOWRITE8(B0.N_and_Q[0], kmb_ocs_aes_dev->base_reg +
			AES_A_DMA_INBUFFER_WRITE_FIFO_OFFSET);
	OCS_IOWRITE8(B0.N_and_Q[1], kmb_ocs_aes_dev->base_reg +
			AES_A_DMA_INBUFFER_WRITE_FIFO_OFFSET);
	OCS_IOWRITE8(B0.N_and_Q[2], kmb_ocs_aes_dev->base_reg +
			AES_A_DMA_INBUFFER_WRITE_FIFO_OFFSET);
	OCS_IOWRITE8(B0.N_and_Q[3], kmb_ocs_aes_dev->base_reg +
			AES_A_DMA_INBUFFER_WRITE_FIFO_OFFSET);
	OCS_IOWRITE8(B0.N_and_Q[4], kmb_ocs_aes_dev->base_reg +
			AES_A_DMA_INBUFFER_WRITE_FIFO_OFFSET);
	OCS_IOWRITE8(B0.N_and_Q[5], kmb_ocs_aes_dev->base_reg +
			AES_A_DMA_INBUFFER_WRITE_FIFO_OFFSET);
	OCS_IOWRITE8(B0.N_and_Q[6], kmb_ocs_aes_dev->base_reg +
			AES_A_DMA_INBUFFER_WRITE_FIFO_OFFSET);
	OCS_IOWRITE8(B0.N_and_Q[7], kmb_ocs_aes_dev->base_reg +
			AES_A_DMA_INBUFFER_WRITE_FIFO_OFFSET);
	OCS_IOWRITE8(B0.N_and_Q[8], kmb_ocs_aes_dev->base_reg +
			AES_A_DMA_INBUFFER_WRITE_FIFO_OFFSET);
	OCS_IOWRITE8(B0.N_and_Q[9], kmb_ocs_aes_dev->base_reg +
			AES_A_DMA_INBUFFER_WRITE_FIFO_OFFSET);
	OCS_IOWRITE8(B0.N_and_Q[10], kmb_ocs_aes_dev->base_reg +
			AES_A_DMA_INBUFFER_WRITE_FIFO_OFFSET);
	OCS_IOWRITE8(B0.N_and_Q[11], kmb_ocs_aes_dev->base_reg +
			AES_A_DMA_INBUFFER_WRITE_FIFO_OFFSET);
	OCS_IOWRITE8(B0.N_and_Q[12], kmb_ocs_aes_dev->base_reg +
			AES_A_DMA_INBUFFER_WRITE_FIFO_OFFSET);
	OCS_IOWRITE8(B0.N_and_Q[13], kmb_ocs_aes_dev->base_reg +
			AES_A_DMA_INBUFFER_WRITE_FIFO_OFFSET);
	OCS_IOWRITE8(B0.N_and_Q[14], kmb_ocs_aes_dev->base_reg +
			AES_A_DMA_INBUFFER_WRITE_FIFO_OFFSET);

	if (aad_size) {
		/* Write AAD size */
		OCS_IOWRITE8((uint8_t)(aad_size/0x100),
				kmb_ocs_aes_dev->base_reg +
				AES_A_DMA_INBUFFER_WRITE_FIFO_OFFSET);
		OCS_IOWRITE8((uint8_t)(aad_size%0x100),
				kmb_ocs_aes_dev->base_reg +
				AES_A_DMA_INBUFFER_WRITE_FIFO_OFFSET);

		/* DMA: Fetch AAD */
		OCS_IOWRITE32(aad_descriptor, kmb_ocs_aes_dev->base_reg +
				AES_A_DMA_NEXT_SRC_DESCR_OFFSET);
		OCS_IOWRITE32(aad_descriptor, kmb_ocs_aes_dev->base_reg +
				AES_A_DMA_NEXT_DST_DESCR_OFFSET);
		OCS_IOWRITE32(0, kmb_ocs_aes_dev->base_reg +
				AES_A_DMA_SRC_SIZE_OFFSET);
		/* DMA mode
		 * bit[31] - ACTIVE
		 * bit[25] - SRC_LINK_LIST_EN
		 */
		OCS_IOWRITE32(0x83000000, kmb_ocs_aes_dev->base_reg +
				AES_A_DMA_DMA_MODE_OFFSET);

		/* set LAST_GCX and LAST_ADATA bit
		 * (only after last batch of AAD)
		 * Write trigger bit
		 * default 32'b0
		 * bit [9] - LAST_ADATA
		 * bit [8] - LAST_GCX
		 */
		OCS_IOWRITE32(0x00000300, kmb_ocs_aes_dev->base_reg +
				AES_ACTIVE_OFFSET);


		/* Poll DMA_MSI_ISR SRC_DONE_INT_STATUS */
		//TODO: replace with interrupt?
		do {
			val = OCS_IOREAD32(kmb_ocs_aes_dev->base_reg +
					AES_A_DMA_MSI_ISR_OFFSET);
		} while ((val & 0x00000001) == 0);
	} else {
		/* Since no aad the LAST_GCX bit can be set now */

		/* set LAST_GCX and LAST_ADATA bit
		 * (only after last batch of AAD)
		 * Write trigger bit
		 * default 32'b0
		 * bit [9] - LAST_ADATA
		 * bit [8] - LAST_GCX
		 */
		OCS_IOWRITE32(0x00000300, kmb_ocs_aes_dev->base_reg +
				AES_ACTIVE_OFFSET);
	}

	/* Poll AES_ACTIVE LAST_GCX */
	do {
		val = OCS_IOREAD32(kmb_ocs_aes_dev->base_reg +
				AES_ACTIVE_OFFSET);
	} while (val & 0x00000100);

	OCS_IOWRITE32(0, kmb_ocs_aes_dev->base_reg + AES_A_DMA_SRC_SIZE_OFFSET);
	OCS_IOWRITE32(0, kmb_ocs_aes_dev->base_reg + AES_A_DMA_DST_SIZE_OFFSET);
	/* Set output address */
	OCS_IOWRITE32(dst_descriptor, kmb_ocs_aes_dev->base_reg +
			AES_A_DMA_NEXT_DST_DESCR_OFFSET);
	if (src_size) {
		/* Fetch plain text */
		OCS_IOWRITE32(src_descriptor, kmb_ocs_aes_dev->base_reg +
				AES_A_DMA_NEXT_SRC_DESCR_OFFSET);
		/* DMA mode
		 * bit[31] - ACTIVE
		 * bit[25] - SRC_LINK_LIST_EN
		 * bit[24] - DST_LINK_LIST_EN
		 */
		OCS_IOWRITE32(0x83000000, kmb_ocs_aes_dev->base_reg +
				AES_A_DMA_DMA_MODE_OFFSET);
	} else {
		if (instruction == AES_ENCRYPT) {
			OCS_IOWRITE32(0x81000000, kmb_ocs_aes_dev->base_reg +
					AES_A_DMA_DMA_MODE_OFFSET);
		} else {
			OCS_IOWRITE32(0x80000000, kmb_ocs_aes_dev->base_reg +
					AES_A_DMA_DMA_MODE_OFFSET);
		}
	}

	/* set LAST_GCX bit
	 * Write trigger bit
	 * default 32'b0
	 * bit [8] - LAST_GCX
	 */
	OCS_IOWRITE32(0x00000100, kmb_ocs_aes_dev->base_reg +
			AES_ACTIVE_OFFSET);

	if (instruction == AES_ENCRYPT) {
		wait_for_completion(&g_aes_complete);
		reinit_completion(&g_aes_complete);
	} else {
		/* Poll DMA_MSI_ISR SRC_DONE_INT_STATUS */
		//TODO: replace with interrupt
		do {
			val = OCS_IOREAD32(kmb_ocs_aes_dev->base_reg +
					AES_A_DMA_MSI_ISR_OFFSET);
		} while ((val & 0x00000001) == 0);

		ccm_hw_cycles_delay(CCM_DECRYPT_DELAY_CLK_COUNT);


		/* write encrypted tag */
		for (i = 0; i < tag_size; i++) {
			OCS_IOWRITE8(in_tag[i], kmb_ocs_aes_dev->base_reg +
					AES_A_DMA_INBUFFER_WRITE_FIFO_OFFSET);
		}

		if (!wait_for_completion_timeout(&g_aes_complete,
					msecs_to_jiffies(10))) {
			pr_info("CCM decrypt completion timedout\n");

			/* TODO: what now?
			 * No apparent way to reset
			 * So should OCS algorithms be unregistered from
			 * crypto API?
			 */
		}
		reinit_completion(&g_aes_complete);
		*auth_res = ccm_compare_tag_to_yr(tag_size);
	}

	aes_irq_disable();

	return 0;
}


//This is a bit contrieved, but useful to test dma linked list operation
//TODO: remove
uint32_t ocs_create_linked_list_from_dma_buffer(uint8_t **unaligned,
		dma_addr_t *descriptor, uint32_t data_size, dma_addr_t data)
{
	struct ocs_dma_linked_list *aligned = NULL;
	uint32_t n; //sgl_num_entries(sgl)
	uint32_t desc_size;
	int i;

//arbitrary size to allow testing of multi-node linked list
//in "normal" circumstances a single dma buffer will result
//in a single node linked list
#define NODE_DATA_SIZE (25)

	if (data_size == 0)
		n = 0;
	else if (data_size % NODE_DATA_SIZE == 0)
		n = data_size / NODE_DATA_SIZE;
	else
		n = (data_size / NODE_DATA_SIZE) + 1;

	if (n == 0) {
		descriptor = NULL;
		*unaligned = NULL;
		return 0;
	}

	desc_size = sizeof(struct ocs_dma_linked_list) * n;

	//HW requirement: descriptor must be 8 byte aligned
	//TODO: valid to assume kmalloc returns suitably
	//aligned address?
	*unaligned = kmalloc(desc_size, GFP_KERNEL);

	aligned = (struct ocs_dma_linked_list *)(*unaligned);

	*descriptor = dma_map_single(&kmb_ocs_aes_dev->plat_dev->dev,
			*unaligned, desc_size, DMA_TO_DEVICE);
	if (dma_mapping_error(&kmb_ocs_aes_dev->plat_dev->dev, *descriptor)) {
		pr_err("ERROR setting up DMA buffer for linked list descriptor\n");
		kfree(unaligned);
		return -1;
	}

	dma_sync_single_for_cpu(&kmb_ocs_aes_dev->plat_dev->dev, *descriptor,
			desc_size, DMA_TO_DEVICE);

	for (i = 0; i < n; i++) {
		aligned[i].address = data + (i*NODE_DATA_SIZE);
		aligned[i].byte_count =
			(data_size - (i * NODE_DATA_SIZE)) > NODE_DATA_SIZE ?
			NODE_DATA_SIZE : data_size - (i * NODE_DATA_SIZE);
		aligned[i].freeze = 0;
		aligned[i].next = *descriptor +
			(sizeof(struct ocs_dma_linked_list) * (i+1));
		aligned[i].reserved = 0;
		aligned[i].terminate = 0;
	}
	aligned[n-1].next = 0;
	aligned[n-1].terminate = 1;

	dma_sync_single_for_device(&kmb_ocs_aes_dev->plat_dev->dev,
			*descriptor, desc_size, DMA_TO_DEVICE);

	return desc_size;
}

void ocs_create_linked_list_from_sg(struct scatterlist *sgl,
		uint32_t num_sgl_entries, uint8_t **aad_buf,
		dma_addr_t *aad_descriptor, uint32_t aad_size,
		uint32_t *aad_desc_size,
		uint8_t **data_buf, dma_addr_t *data_descriptor,
		uint32_t data_size, uint32_t *data_desc_size)
{
	struct ocs_dma_linked_list *ll = NULL;
	uint32_t data_offset = 0;
	struct scatterlist *sg;
	int num_aad_ents, i;

	if (num_sgl_entries == 0)
		goto ret_err;

	sg = sgl;

	if (aad_size) {
		num_aad_ents = sg_nents_for_len(sgl, aad_size);
		if (num_aad_ents < 0)
			goto ret_err;

		*aad_desc_size = sizeof(struct ocs_dma_linked_list) *
			num_aad_ents;

		/* HW requirement: descriptor must be 8 byte aligned */
		//TODO: valid to assume kmalloc returns suitably
		//aligned address?
		*aad_buf = kmalloc(*aad_desc_size, GFP_KERNEL);

		ll = (struct ocs_dma_linked_list *)(*aad_buf);

		*aad_descriptor = dma_map_single(
				&kmb_ocs_aes_dev->plat_dev->dev, *aad_buf,
				*aad_desc_size, DMA_TO_DEVICE);
		if (dma_mapping_error(&kmb_ocs_aes_dev->plat_dev->dev,
					*aad_descriptor)) {
			pr_err("ERROR setting up DMA buffer for linked list aad descriptor\n");
			*aad_descriptor = 0;
			goto ret_err;
		}

		dma_sync_single_for_cpu(&kmb_ocs_aes_dev->plat_dev->dev,
				*aad_descriptor, *aad_desc_size, DMA_TO_DEVICE);

		i = 0;
		while (true) {
			ll[i].address = sg_dma_address(sg);
			ll[i].byte_count = (sg_dma_len(sg) < aad_size) ?
				sg_dma_len(sg) : aad_size;
			aad_size -= ll[i].byte_count;
			ll[i].freeze = 0;
			ll[i].next = *aad_descriptor +
				(sizeof(struct ocs_dma_linked_list) * (i+1));
			ll[i].reserved = 0;
			ll[i].terminate = 0;
			i++;
			if (aad_size && (i < num_aad_ents))
				sg = sg_next(sg);
			else
				break;
		}
		ll[i-1].next = 0;
		ll[i-1].terminate = 1;
		data_offset = ll[i-1].byte_count;

		dma_sync_single_for_device(&kmb_ocs_aes_dev->plat_dev->dev,
				*aad_descriptor, *aad_desc_size, DMA_TO_DEVICE);
	} else {
		num_aad_ents = 0;
	}

	if (data_size) {
		/* +1 for case where aad and data overlap in one sgl node */
		num_sgl_entries = num_sgl_entries - num_aad_ents + 1;

		*data_desc_size = sizeof(struct ocs_dma_linked_list) *
			num_sgl_entries;

		/* HW requirement: descriptor must be 8 byte aligned */
		//TODO: valid to assume kmalloc returns suitably
		//aligned address?
		*data_buf = kmalloc(*data_desc_size, GFP_KERNEL);

		ll = (struct ocs_dma_linked_list *)(*data_buf);

		*data_descriptor = dma_map_single(
				&kmb_ocs_aes_dev->plat_dev->dev, *data_buf,
				*data_desc_size, DMA_TO_DEVICE);
		if (dma_mapping_error(&kmb_ocs_aes_dev->plat_dev->dev,
					*data_descriptor)) {
			pr_err("ERROR setting up DMA buffer for linked list data descriptor\n");
			goto ret_err;
		}

		dma_sync_single_for_cpu(&kmb_ocs_aes_dev->plat_dev->dev,
				*data_descriptor, *data_desc_size,
				DMA_TO_DEVICE);

		if (data_offset == sg_dma_len(sg)) {
			sg = sg_next(sg);
			data_offset = 0;
		}

		for (i = 0; (i < num_sgl_entries) && data_size;
				i++, sg = sg_next(sg)) {
			ll[i].address = sg_dma_address(sg) + data_offset;
			ll[i].byte_count =
				((sg_dma_len(sg) - data_offset) < data_size) ?
				(sg_dma_len(sg) - data_offset) : data_size;
			data_offset = 0;
			data_size -= ll[i].byte_count;
			ll[i].freeze = 0;
			ll[i].next = *data_descriptor +
				(sizeof(struct ocs_dma_linked_list) * (i+1));
			ll[i].reserved = 0;
			ll[i].terminate = 0;
		}
		ll[i-1].next = 0;
		ll[i-1].terminate = 1;

		dma_sync_single_for_device(&kmb_ocs_aes_dev->plat_dev->dev,
				*data_descriptor, *data_desc_size,
				DMA_TO_DEVICE);
	}

	return;

ret_err:
		if (data_buf && *data_buf)
			kfree(*data_buf);
		data_descriptor = NULL;
		if (data_buf)
			*data_buf = NULL;
		*data_desc_size = 0;

		if (*aad_descriptor)
			dma_unmap_single(&kmb_ocs_aes_dev->plat_dev->dev,
					*aad_descriptor, *aad_desc_size,
					DMA_TO_DEVICE);
		if (aad_buf && *aad_buf)
			kfree(*aad_buf);
		if (aad_buf)
			*aad_buf = NULL;
		*aad_desc_size = 0;

		return;
}

MODULE_LICENSE("GPL v2");
