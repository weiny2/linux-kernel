/* SPDX-License-Identifier: GPL-2.0-only */
/*
 * KeemBay OCS AES Crypto Driver.
 *
 * Copyright (C) 2018-2019 Intel Corporation
 */

#ifndef _CRYPTO_OCS_AES_H
#define _CRYPTO_OCS_AES_H

#include <linux/module.h>
#include <linux/irqreturn.h>
#include <linux/scatterlist.h>

#define OCS_DEBUG_VERBOSE (1)

#define AES_BLOCK_SIZE	(16)

enum aes_instruction {
	AES_ENCRYPT = 0,
	AES_DECRYPT = 1,
	AES_EXPAND = 2,
	AES_BYPASS = 3,
};

enum aes_mode {
	AES_MODE_ECB = 0,
	AES_MODE_CBC = 1,
	AES_MODE_CTR = 2,
	AES_MODE_CCM = 6,
	AES_MODE_GCM = 7,
	AES_MODE_CTS = 9,
};

struct ocs_dma_linked_list {
	uint32_t address;
	uint32_t byte_count;
	uint32_t next;
	uint32_t reserved  :30;
	uint32_t freeze    :1;
	uint32_t terminate :1;
};

void ocs_wrapper_init(uint64_t base_address);

int ocs_aes_set_key(const uint32_t key_size, const uint8_t *key);

//if present IV must be 16 bytes
int ocs_aes_op(const dma_addr_t src_descriptor, const uint32_t src_size,
		const uint8_t *iv, const uint32_t iv_size,
		const enum aes_mode mode,
		const enum aes_instruction instruction,
		const dma_addr_t dst_descriptor);

int ocs_aes_gcm_op(const dma_addr_t src_descriptor, const uint32_t src_size,
		const uint8_t *iv, const uint32_t iv_size,
		const dma_addr_t aad_descriptor, const uint32_t aad_size,
		const uint32_t tag_size, const enum aes_instruction instruction,
		const dma_addr_t dst_descriptor, uint8_t *tag);

/* nonce must be converted to CCM IV (expected/provided by crypto API)
 * For encrypt the tag is appended to the ciphertext in destination
 * For decrypt the authentication result is found in auth_res
 *	0 = authentication successful
 *	1 = authentication failed
 */
int ocs_aes_ccm_op(const dma_addr_t src_descriptor, const uint32_t src_size,
		const uint8_t *iv,
		const dma_addr_t aad_descriptor, const uint32_t aad_size,
		const uint8_t *in_tag, const uint32_t tag_size,
		const enum aes_instruction instruction,
		const dma_addr_t dst_descriptor, uint8_t *auth_res);

/* run kfree() on unaligned when finished with descriptor */
uint32_t ocs_create_linked_list_from_dma_buffer(uint8_t **unaligned,
		dma_addr_t *descriptor, uint32_t data_size, dma_addr_t data);

/* run kfree() on data_buf and aad_buf when finished with descriptor
 * data_descriptor, data_desc_size, aad_descriptor and aad_desc_size are output
 */
void ocs_create_linked_list_from_sg(struct scatterlist *sgl,
		uint32_t num_sgl_entries, uint8_t **aad_buf,
		dma_addr_t *aad_descriptor, uint32_t aad_size,
		uint32_t *aad_desc_size, uint8_t **data_buf,
		dma_addr_t *data_descriptor, uint32_t data_size,
		uint32_t *data_desc_size);

irqreturn_t ocs_aes_irq_handler(int irq, void *dev_id);

#endif
