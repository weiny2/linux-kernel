// SPDX-License-Identifier: GPL-2.0-only
/*
 * KeemBay OCS AES Crypto Driver.
 *
 * Copyright (C) 2018-2019 Intel Corporation
 */

#define pr_fmt(fmt) KBUILD_MODNAME ": " fmt

#include <linux/module.h>
#include <linux/types.h>
#include <linux/of.h>
#include <linux/io.h>
#include <linux/platform_device.h>
#include <linux/interrupt.h>
#include <linux/clk.h>
#include <linux/crypto.h>
#include <crypto/internal/skcipher.h>
#include <crypto/internal/aead.h>
#include <crypto/scatterwalk.h>
#include "thunderbay-ocs-aes-core.h"
#include "ocs-aes.h"

#define KMB_OCS_PRIORITY (350)
#define DRV_NAME "thunderbay-ocs-aes-driver"
#define OCS_AES_VERSION_MAJOR	0
#define OCS_AES_VERSION_MINOR	6

#define AES_MIN_KEY_SIZE		16
#define AES_MAX_KEY_SIZE		32
#define AES_KEYSIZE_128			16
#define AES_KEYSIZE_256			32
#define GCM_AES_IV_SIZE			12

static int __init register_aes_algs(void);
static void __exit unregister_aes_algs(void);

struct keembay_ocs_aes_dev *kmb_ocs_aes_dev;

/**
 * ocs_aes_hw_init() = Init OCS AES-related hardware and memory.
 */
static int ocs_aes_hw_init(struct platform_device *pdev,
		struct keembay_ocs_aes_dev *ocs_aes_dev)
{
	int rc, irq;
	struct resource *res;
	void __iomem *reg;
	//struct clk *clk;

	/* Get ocs base register address. */
	res = platform_get_resource_byname(pdev, IORESOURCE_MEM, "ocs_base");
	if (!res) {
		dev_err(&pdev->dev, "Couldn't find register address\n");
		return -EINVAL;
	}
	reg = devm_ioremap_resource(&pdev->dev, res);
	if (IS_ERR(reg)) {
		dev_err(&pdev->dev, "Failed to get ocs base address\n");
		return PTR_ERR(reg);
	}
	ocs_aes_dev->base_reg = reg;

/* Get base register address. */
	res = platform_get_resource_byname(pdev, IORESOURCE_MEM, "ocs_wrapper_base");
	if (!res) {
		dev_err(&pdev->dev, "Couldn't find register address\n");
		return -EINVAL;
	}
	reg = devm_ioremap_resource(&pdev->dev, res);
	if (IS_ERR(reg)) {
		dev_err(&pdev->dev, "Failed to get ocs wrapper base address\n");
		return PTR_ERR(reg);
	}
	ocs_aes_dev->base_wrapper_reg = reg;

	irq = platform_get_irq(pdev, 0);
	if (irq < 0)
		dev_err(&pdev->dev,
				"Failed getting IRQ for OCS AES: %d\n", irq);
	ocs_aes_dev->irq = irq;
	ocs_aes_dev->irq_enabled = true;
	spin_lock_init(&ocs_aes_dev->irq_lock);
	dev_info(&pdev->dev, "Registering handler for IRQ %d\n", irq);
	rc = devm_request_irq(&pdev->dev, irq, ocs_aes_irq_handler, 0,
			"keembay-ocs-aes", ocs_aes_dev);

 ocs_wrapper_init(0x1700000000);

#if 0  //TODO: Enable when clk is available on THB dtb
	clk = devm_clk_get(&pdev->dev, NULL);
	if (IS_ERR(clk)) {
		dev_err(&pdev->dev, "Failed to get OCS clock\n");
		return PTR_ERR(clk);
	}
	ocs_aes_dev->ocs_clk = clk;

	dev_info(&pdev->dev, "OCS clk rate %luHz\n",
			clk_get_rate(ocs_aes_dev->ocs_clk));
#endif

	return 0;
}


/* Driver probing. */
static int kmb_ocs_aes_probe(struct platform_device *pdev)
{
	int ret;

	dev_info(&pdev->dev, "Keembay OCS AES v%d.%d\n", OCS_AES_VERSION_MAJOR,
			OCS_AES_VERSION_MINOR);

	ret = dma_set_mask_and_coherent(&pdev->dev, DMA_BIT_MASK(64));
	if (ret) {
		dev_err(&pdev->dev, "Failed to set 64 bit dma mask %d\n", ret);
		return -ENODEV;
	}

	kmb_ocs_aes_dev = devm_kzalloc(&pdev->dev,
			sizeof(*kmb_ocs_aes_dev), GFP_KERNEL);
	if (!kmb_ocs_aes_dev)
		return -ENOMEM;

	kmb_ocs_aes_dev->plat_dev = pdev;

	ocs_aes_hw_init(pdev, kmb_ocs_aes_dev);

	platform_set_drvdata(pdev, kmb_ocs_aes_dev);

	register_aes_algs();

	return 0;
}

/* Driver removal. */
static int kmb_ocs_aes_remove(struct platform_device *pdev)
{
	/*
	 * Nothing to do as iomem and irq have been requested with
	 * devm_* functions and, therefore, are freed automatically.
	 */

	unregister_aes_algs();

	return 0;
}

/* Device tree driver match. */
static const struct of_device_id kmb_ocs_aes_of_match[] = {
	{
		.compatible = "intel,keembay-ocs-aes",
	},
	{}
};

/* The OCS driver is a platform device. */
static struct platform_driver kmb_ocs_aes_driver = {
	.probe = kmb_ocs_aes_probe,
	.remove = kmb_ocs_aes_remove,
	.driver = {
			.name = DRV_NAME,
			.of_match_table = kmb_ocs_aes_of_match,
		},
};

module_platform_driver(kmb_ocs_aes_driver);


/********************* CRYPTO API HERE *********************/
//TODO: Will these, or similar, be needed when queuing?
//struct ocs_aes_ctx {
//    int keylen;
//    u32 key[AES_KEYSIZE_256 / sizeof(u32)];
//};

//struct ocs_aes_reqctx {
//    unsigned long mode;
//    u8 iv[AES_BLOCK_SIZE];
//};

static int kmb_ocs_aes_set_key(struct crypto_skcipher *tfm, const u8 *in_key,
		       unsigned int key_len)
{
	//struct ocs_aes_ctx *ctx = crypto_skcipher_ctx(tfm);

	if (key_len != AES_KEYSIZE_128 &&
			key_len != AES_KEYSIZE_256) {
		//TODO: handle fallback for AES_192??
		//see omap_aes_setkey() in omap-aes.c
		pr_info("Bad input key len\n");
		return -EINVAL;
	}

	ocs_aes_set_key(key_len, in_key);

	return 0;
}

static int kmb_ocs_aes_aead_set_key(struct crypto_aead *tfm, const u8 *in_key,
		       unsigned int key_len)
{
	if (key_len != AES_KEYSIZE_128 &&
			key_len != AES_KEYSIZE_256) {
		//TODO: handle fallback for AES_192??
		//see omap_aes_setkey() in omap-aes.c
		pr_info("Bad input key len\n");
		return -EINVAL;
	}

	ocs_aes_set_key(key_len, in_key);

	return 0;
}

static inline void sg_swap_blocks(struct scatterlist *sgl, unsigned int nents,
		off_t blk1_offset, off_t blk2_offset)
{
	uint8_t tmp_buf1[AES_BLOCK_SIZE], tmp_buf2[AES_BLOCK_SIZE];

	/* No easy way to copy within sg list, so copy both
	 * blocks to temporary buffers first
	 */
	sg_pcopy_to_buffer(sgl, nents, tmp_buf1, AES_BLOCK_SIZE, blk1_offset);
	sg_pcopy_to_buffer(sgl, nents, tmp_buf2, AES_BLOCK_SIZE, blk2_offset);
	sg_pcopy_from_buffer(sgl, nents, tmp_buf1, AES_BLOCK_SIZE, blk2_offset);
	sg_pcopy_from_buffer(sgl, nents, tmp_buf2, AES_BLOCK_SIZE, blk1_offset);
}

static int aes_skcipher_common(struct skcipher_request *req,
		const enum aes_instruction instruction,
		const enum aes_mode mode)
{
	uint8_t *src_desc_buf = NULL, *dst_desc_buf = NULL;
	uint32_t src_desc_size = 0, dst_desc_size = 0;
	dma_addr_t src_desc = 0, dst_desc = 0;
	int num_src = 0, num_dst;
	int ret = 0, in_place, cts_swap, iv_size;
	uint8_t last_c[AES_BLOCK_SIZE];

	iv_size = crypto_skcipher_ivsize(crypto_skcipher_reqtfm(req));

	num_dst =  sg_nents_for_len(req->dst, req->cryptlen);
	if (num_dst < 0) {
		ret = -EBADMSG;
		goto ret_func;
	}

	if ((mode == AES_MODE_CTS) &&
			(req->cryptlen > AES_BLOCK_SIZE) &&
			(!(req->cryptlen % AES_BLOCK_SIZE))) {
		/* If 2 blocks or greater, and multiple of block size
		 * swap last two blocks to be compatible with other
		 * crypto API CTS implementations
		 * i.e. from https://nvlpubs.nist.gov/nistpubs/Legacy/SP/
		 * nistspecialpublication800-38a-add.pdf OCS mode uses
		 * CBC-CS2, whereas other crypto API implementations use
		 * CBC-CS3
		 */
		cts_swap = 1;
	} else
		cts_swap = 0;

	if ((req->src->page_link == req->dst->page_link) &&
	   (req->src->offset == req->dst->offset)) {
		/* src and dst buffers are the same,
		 * so must use bidirectional DMA mapping
		 */
		in_place = 1;

		if ((instruction == AES_DECRYPT) &&
				(mode == AES_MODE_CBC))
			scatterwalk_map_and_copy(last_c, req->src,
					(req->cryptlen - iv_size), iv_size, 0);

		if (cts_swap &&	(instruction == AES_DECRYPT))
			sg_swap_blocks(req->dst, num_dst,
					(req->cryptlen - AES_BLOCK_SIZE),
					(req->cryptlen - (2 * AES_BLOCK_SIZE)));

		ret = dma_map_sg(&kmb_ocs_aes_dev->plat_dev->dev, req->dst,
				num_dst, DMA_BIDIRECTIONAL);

	} else {
		in_place = 0;

		num_src =  sg_nents_for_len(req->src, req->cryptlen);
		if (num_src < 0) {
			ret = -EBADMSG;
			goto ret_func;
		}

		ret = dma_map_sg(&kmb_ocs_aes_dev->plat_dev->dev, req->src,
				num_src, DMA_TO_DEVICE);
		if (ret != num_src) {
			dev_err(&kmb_ocs_aes_dev->plat_dev->dev, "Failed to map source sg\n");
			ret = -EINVAL;
			goto unmap_src;
		}
		ocs_create_linked_list_from_sg(req->src, num_src,
				NULL, NULL, 0, NULL,
				(uint8_t **)&src_desc_buf, &src_desc,
				req->cryptlen, &src_desc_size);

		ret = dma_map_sg(&kmb_ocs_aes_dev->plat_dev->dev, req->dst,
				num_dst, DMA_FROM_DEVICE);
	}

	if (ret != num_dst) {
		dev_err(&kmb_ocs_aes_dev->plat_dev->dev, "Failed to map destination sg\n");
		ret = -EINVAL;
		goto unmap_dst;
	}
	ocs_create_linked_list_from_sg(req->dst, num_dst,
			NULL, NULL, 0, NULL,
			(uint8_t **)&dst_desc_buf, &dst_desc,
			req->cryptlen, &dst_desc_size);

	if (in_place && dst_desc_size) {
		src_desc = dst_desc;
	} else if (!src_desc_size || !dst_desc_size) {
		ret = -1;
		goto unmap_dst;
	}

	if ((!in_place) && cts_swap && (instruction == AES_DECRYPT)) {
		/* Not in place, so have to copy src to dst (as we can't
		 * modify src)
		 * The mode (AES_MODE_ECB) is ignored when the instruction
		 * is AES_BYPASS
		 * TODO: for anything other than small data sizes this is
		 * very inefficient. Ideally everything other than last 2
		 * blocks would be read from src for decryption, and last
		 * 2 blocks swapped and read from elsewhere. But currently
		 * implemented OCS API does not allow this
		 */
		ret = ocs_aes_op(src_desc, req->cryptlen, NULL, 0, AES_MODE_ECB,
				AES_BYPASS, dst_desc);
		if (ret)
			goto unmap_dst;

		/* unmap src and dst */
		dma_unmap_sg(&kmb_ocs_aes_dev->plat_dev->dev, req->src,
				num_src, DMA_TO_DEVICE);
		num_src = 0; /* no longer needed */
		dma_unmap_sg(&kmb_ocs_aes_dev->plat_dev->dev, req->dst,
				num_dst, DMA_FROM_DEVICE);

		sg_swap_blocks(req->dst, num_dst,
				(req->cryptlen - AES_BLOCK_SIZE),
				(req->cryptlen - (2 * AES_BLOCK_SIZE)));

		/* remap dst, but now in_place */
		ret = dma_map_sg(&kmb_ocs_aes_dev->plat_dev->dev, req->dst,
				num_dst, DMA_BIDIRECTIONAL);
		in_place = 1;
		if (ret != num_dst) {
			dev_err(&kmb_ocs_aes_dev->plat_dev->dev, "Failed to map destination sg\n");
			ret = -EINVAL;
			goto unmap_dst;
		}

		/* discard existing linked lists */
		if (src_desc_size) {
			dma_unmap_single(&kmb_ocs_aes_dev->plat_dev->dev,
					src_desc, src_desc_size, DMA_TO_DEVICE);
			kfree(src_desc_buf);
			src_desc_size = 0;
		}
		if (dst_desc_size) {
			dma_unmap_single(&kmb_ocs_aes_dev->plat_dev->dev,
					dst_desc, dst_desc_size, DMA_TO_DEVICE);
			kfree(dst_desc_buf);
			dst_desc_size = 0;
		}

		/* need to recreate dst linked list */
		ocs_create_linked_list_from_sg(req->dst, num_dst,
				NULL, NULL, 0, NULL,
				(uint8_t **)&dst_desc_buf, &dst_desc,
				req->cryptlen, &dst_desc_size);

		if (dst_desc_size) {
			/* If descriptor creation was successful,
			 * point the src_desc at the dst_desc,
			 * as we do in-place AES operation on the src
			 */
			src_desc = dst_desc;
		} else {
			ret = -1;
			goto unmap_dst;
		}
	}

	if (mode == AES_MODE_ECB)
		ret = ocs_aes_op(src_desc, req->cryptlen, NULL, 0, mode,
				instruction, dst_desc);
	else
		ret = ocs_aes_op(src_desc, req->cryptlen, req->iv,
				AES_BLOCK_SIZE, mode, instruction, dst_desc);

unmap_dst:
	if (num_src > 0)
		dma_unmap_sg(&kmb_ocs_aes_dev->plat_dev->dev, req->src,
				num_src, DMA_TO_DEVICE);

unmap_src:
	if (num_dst > 0) {
		if (in_place)
			dma_unmap_sg(&kmb_ocs_aes_dev->plat_dev->dev, req->dst,
					num_dst, DMA_BIDIRECTIONAL);
		else
			dma_unmap_sg(&kmb_ocs_aes_dev->plat_dev->dev, req->dst,
					num_dst, DMA_FROM_DEVICE);
	}

	if (src_desc_size) {
		dma_unmap_single(&kmb_ocs_aes_dev->plat_dev->dev, src_desc,
				src_desc_size, DMA_TO_DEVICE);
		kfree(src_desc_buf);
	}

	if (dst_desc_size) {
		dma_unmap_single(&kmb_ocs_aes_dev->plat_dev->dev, dst_desc,
				dst_desc_size, DMA_TO_DEVICE);
		kfree(dst_desc_buf);
	}

	if (!ret && (mode == AES_MODE_CBC)) {
		if (instruction == AES_ENCRYPT) {
			scatterwalk_map_and_copy(req->iv, req->dst,
				req->cryptlen - iv_size, iv_size, 0);
		} else {
			if (in_place) {
				memcpy(req->iv, last_c, iv_size);
			} else {
				scatterwalk_map_and_copy(req->iv, req->src,
					req->cryptlen - iv_size, iv_size, 0);
			}
		}
	}

	if ((!ret) && cts_swap && (instruction == AES_ENCRYPT))
		sg_swap_blocks(req->dst, num_dst,
				(req->cryptlen - AES_BLOCK_SIZE),
				(req->cryptlen - (2 * AES_BLOCK_SIZE)));

ret_func:
	return ret;
}


static int kmb_ocs_aes_ecb_encrypt(struct skcipher_request *req)
{
	//pr_info("In %s\n", __func__);	//TODO: remove
	return aes_skcipher_common(req, AES_ENCRYPT, AES_MODE_ECB);
}

static int kmb_ocs_aes_ecb_decrypt(struct skcipher_request *req)
{
	//pr_info("In %s\n", __func__);	//TODO: remove
	return aes_skcipher_common(req, AES_DECRYPT, AES_MODE_ECB);
}

static int kmb_ocs_aes_cbc_encrypt(struct skcipher_request *req)
{
	//pr_info("In %s\n", __func__);	//TODO: remove
	return aes_skcipher_common(req, AES_ENCRYPT, AES_MODE_CBC);
}

static int kmb_ocs_aes_cbc_decrypt(struct skcipher_request *req)
{
	//pr_info("In %s\n", __func__);	//TODO: remove
	return aes_skcipher_common(req, AES_DECRYPT, AES_MODE_CBC);
}

static int kmb_ocs_aes_ctr_encrypt(struct skcipher_request *req)
{
	//pr_info("In %s\n", __func__);	//TODO: remove
	return aes_skcipher_common(req, AES_ENCRYPT, AES_MODE_CTR);
}

static int kmb_ocs_aes_ctr_decrypt(struct skcipher_request *req)
{
	//pr_info("In %s\n", __func__);	//TODO: remove
	return aes_skcipher_common(req, AES_DECRYPT, AES_MODE_CTR);
}

static int kmb_ocs_aes_cts_encrypt(struct skcipher_request *req)
{
	//pr_info("In %s\n", __func__);	//TODO: remove
	return aes_skcipher_common(req, AES_ENCRYPT, AES_MODE_CTS);
}

static int kmb_ocs_aes_cts_decrypt(struct skcipher_request *req)
{
	//pr_info("In %s\n", __func__);	//TODO: remove
	return aes_skcipher_common(req, AES_DECRYPT, AES_MODE_CTS);
}


static int aes_aead_cipher_common(struct aead_request *req,
		const enum aes_instruction instruction,
		const enum aes_mode mode)
{
	dma_addr_t src_desc = 0, aad_src_desc = 0,
			aad_dst_desc = 0, dst_desc = 0;
	uint8_t *src_desc_buf = NULL, *aad_src_desc_buf = NULL,
			*aad_dst_desc_buf = NULL, *dst_desc_buf = NULL;
	uint32_t src_desc_size = 0, aad_src_desc_size = 0,
			 aad_dst_desc_size = 0, dst_desc_size = 0;
	uint32_t tag_size, in_size, out_size;
	uint8_t out_tag[AES_BLOCK_SIZE], in_tag[AES_BLOCK_SIZE];
	int num_src = 0, num_dst;
	int ret, in_place;
	uint8_t ccm_auth_res = 1;

	/* For encrypt:
	 *		src sg list:
	 *			AAD|PT
	 *		dst sg list expects:
	 *			AAD|CT|tag
	 * For decrypt:
	 *		src sg list:
	 *			AAD|CT|tag
	 *		dst sg list expects:
	 *			AAD|PT
	 */

	tag_size = crypto_aead_authsize(crypto_aead_reqtfm(req));

	num_src = sg_nents_for_len(req->src, (req->assoclen + req->cryptlen));
	if (num_src < 0) {
		ret = -EBADMSG;
		goto ret_func;
	}

	/* for decrypt req->cryptlen includes both ciphertext size and
	 * tag size. However for encrypt it only contains the plaintext
	 * size
	 */
	in_size = req->cryptlen;
	if (instruction == AES_DECRYPT) {
		in_size -= tag_size;

		/* copy tag from source scatter-gather list to
		 * in_tag buffer, before DMA mapping, as will
		 * be required later
		 */
		sg_pcopy_to_buffer(req->src, num_src, in_tag,
				tag_size, req->assoclen + in_size);

		out_size = in_size;
	} else {
		/* In CCM mode the OCS engine appends the tag to the ciphertext,
		 * but in GCM mode the tag must be read from the tag registers
		 * and appended manually below
		 */
		out_size = (mode == AES_MODE_CCM) ?
			(in_size + tag_size) : in_size;
	}

	num_dst = sg_nents_for_len(req->dst,
			(req->assoclen + in_size + tag_size));
	if (num_dst < 0) {
		ret = -EBADMSG;
		goto ret_func;
	}

	if ((req->src->page_link == req->dst->page_link) &&
	   (req->src->offset == req->dst->offset)) {
		/* src and dst buffers are the same,
		 * so must use bidirectional DMA mapping
		 */
		in_place = 1;

		/* no longer needed as not mapping src separately */
		num_src = 0;

		ret = dma_map_sg(&kmb_ocs_aes_dev->plat_dev->dev, req->dst,
				num_dst, DMA_BIDIRECTIONAL);
	} else {
		in_place = 0;

		ret = dma_map_sg(&kmb_ocs_aes_dev->plat_dev->dev, req->src,
				num_src, DMA_TO_DEVICE);
		if (ret != num_src) {
			dev_err(&kmb_ocs_aes_dev->plat_dev->dev, "Failed to map source sg\n");
			ret = -EINVAL;
			goto unmap_src;
		}
		ocs_create_linked_list_from_sg(req->src, num_src,
				(uint8_t **)&aad_src_desc_buf, &aad_src_desc,
				req->assoclen, &aad_src_desc_size,
				(uint8_t **)&src_desc_buf, &src_desc, in_size,
				&src_desc_size);

		ret = dma_map_sg(&kmb_ocs_aes_dev->plat_dev->dev, req->dst,
				num_dst, DMA_FROM_DEVICE);
	}

	if (ret != num_dst) {
		dev_err(&kmb_ocs_aes_dev->plat_dev->dev, "Failed to map destination sg\n");
		ret = -EINVAL;
		goto unmap_dst;
	}

	/* aad_dst_desc to be used to copy aad from src to dst */
	ocs_create_linked_list_from_sg(req->dst, num_dst,
			(uint8_t **)&aad_dst_desc_buf, &aad_dst_desc,
			req->assoclen, &aad_dst_desc_size,
			(uint8_t **)&dst_desc_buf, &dst_desc, out_size,
			&dst_desc_size);

	if (in_place && in_size &&
			(mode == AES_MODE_CCM) &&
			(instruction == AES_ENCRYPT)) {
		/* For CCM encrypt the input and output linked lists contain
		 * different amounts of data.
		 * If !in_place already dealt with above
		 * If !in_size the source linked list is never used
		 */
		ocs_create_linked_list_from_sg(req->dst, num_dst,
				(uint8_t **)&aad_src_desc_buf, &aad_src_desc,
				req->assoclen, &aad_src_desc_size,
				(uint8_t **)&src_desc_buf, &src_desc, in_size,
				&src_desc_size);
	} else if (in_place && ((dst_desc_size && in_size) ||
				(aad_dst_desc_size && req->assoclen))) {
		src_desc = dst_desc;
		aad_src_desc = aad_dst_desc;
	} else if ((in_size && (!src_desc_size || !dst_desc_size)) ||
			(req->assoclen && !aad_src_desc_size)) {
		ret = -1;
		goto unmap_dst;
	} else if (req->assoclen && aad_src_desc_size && aad_dst_desc_size) {
		/* copy aad from src to dst using OCS DMA
		 * the mode (AES_MODE_ECB) is ignored when
		 * the instruction is AES_BYPASS
		 */
		ret = ocs_aes_op(aad_src_desc, in_size, NULL, 0, AES_MODE_ECB,
				AES_BYPASS, aad_dst_desc);
		if (ret) {
			dev_err(&kmb_ocs_aes_dev->plat_dev->dev,
					"Unable to copy source AAD to destination AAD\n");
			goto unmap_dst;
		}

	}

	if (mode == AES_MODE_GCM) {
		ret = ocs_aes_gcm_op(src_desc, in_size,
				req->iv, GCM_AES_IV_SIZE,
				aad_src_desc, req->assoclen, tag_size,
				instruction, dst_desc, out_tag);
	} else {
		ret = ocs_aes_ccm_op(src_desc, in_size, req->iv,
				aad_src_desc, req->assoclen, in_tag, tag_size,
				instruction, dst_desc, &ccm_auth_res);
	}

unmap_dst:
	if (num_src > 0)
		dma_unmap_sg(&kmb_ocs_aes_dev->plat_dev->dev, req->src,
				num_src, DMA_TO_DEVICE);

unmap_src:
	if (num_dst > 0) {
		if (in_place)
			dma_unmap_sg(&kmb_ocs_aes_dev->plat_dev->dev, req->dst,
					num_dst, DMA_BIDIRECTIONAL);
		else
			dma_unmap_sg(&kmb_ocs_aes_dev->plat_dev->dev, req->dst,
					num_dst, DMA_FROM_DEVICE);
	}

	if (aad_src_desc_size) {
		dma_unmap_single(&kmb_ocs_aes_dev->plat_dev->dev, aad_src_desc,
				aad_src_desc_size, DMA_TO_DEVICE);
		kfree(aad_src_desc_buf);
	}

	if (aad_dst_desc_size) {
		dma_unmap_single(&kmb_ocs_aes_dev->plat_dev->dev, aad_dst_desc,
				aad_dst_desc_size, DMA_TO_DEVICE);
		kfree(aad_dst_desc_buf);
	}

	if (src_desc_size) {
		dma_unmap_single(&kmb_ocs_aes_dev->plat_dev->dev, src_desc,
				src_desc_size, DMA_TO_DEVICE);
		kfree(src_desc_buf);
	}

	if (dst_desc_size) {
		dma_unmap_single(&kmb_ocs_aes_dev->plat_dev->dev, dst_desc,
				dst_desc_size, DMA_TO_DEVICE);
		kfree(dst_desc_buf);
	}

	if (!ret) {
		if (mode == AES_MODE_GCM) {
			if (instruction == AES_DECRYPT) {
				if (memcmp(in_tag, out_tag, tag_size))
					ret = -EBADMSG;
			}

			sg_pcopy_from_buffer(req->dst, num_dst, out_tag,
					tag_size, req->assoclen + in_size);
		} else if ((mode == AES_MODE_CCM) &&
				(instruction == AES_DECRYPT)) {
			ret = ccm_auth_res ? -EBADMSG : 0;
		}
	}

ret_func:
	//TODO: restore???
	//req->base.complete(&req->base, ret);
	return ret;
}

static int kmb_ocs_aes_gcm_encrypt(struct aead_request *req)
{
	//pr_info("In %s\n", __func__);	//TODO: remove
	return aes_aead_cipher_common(req, AES_ENCRYPT, AES_MODE_GCM);
}

static int kmb_ocs_aes_gcm_decrypt(struct aead_request *req)
{
	//pr_info("In %s\n", __func__);	//TODO: remove
	return aes_aead_cipher_common(req, AES_DECRYPT, AES_MODE_GCM);
}

static int kmb_ocs_aes_ccm_encrypt(struct aead_request *req)
{
	//pr_info("In %s\n", __func__);	//TODO: remove
	return aes_aead_cipher_common(req, AES_ENCRYPT, AES_MODE_CCM);
}

static int kmb_ocs_aes_ccm_decrypt(struct aead_request *req)
{
	//pr_info("In %s\n", __func__);	//TODO: remove
	return aes_aead_cipher_common(req, AES_DECRYPT, AES_MODE_CCM);
}

static void ocs_aes_cra_exit(struct crypto_skcipher *tfm)
{
	const uint8_t zero_key[AES_KEYSIZE_256] = {0};

	/* zero key registers */
	ocs_aes_set_key(AES_KEYSIZE_256, zero_key);
}

static void ocs_aes_aead_cra_exit(struct crypto_aead *tfm)
{
//	ocs_aes_cra_exit(crypto_aead_tfm(tfm));
	const uint8_t zero_key[AES_KEYSIZE_256] = {0};

	/* zero key registers */
	ocs_aes_set_key(AES_KEYSIZE_256, zero_key);

}

static struct skcipher_alg algs[] = {
	{
  .base = {
		 .cra_name = "ecb(aes)",
		 .cra_driver_name = "ecb-aes-keembay-ocs",
		 .cra_priority = KMB_OCS_PRIORITY,
		 .cra_flags = CRYPTO_ALG_ASYNC |
					 CRYPTO_ALG_NEED_FALLBACK | //TODO:???
					 CRYPTO_ALG_KERN_DRIVER_ONLY,
		 .cra_blocksize = AES_BLOCK_SIZE,
		//.cra_ctxsize = sizeof(struct ocs_aes_ctx), //TODO: required?
		 .cra_ctxsize = 0,
		 .cra_alignmask = 0,
		 .cra_module = THIS_MODULE,
  },
		//.init = ocs_aes_cra_init, //TODO: needed?
		.exit = ocs_aes_cra_exit,
		.min_keysize = AES_MIN_KEY_SIZE,
		.max_keysize = AES_MAX_KEY_SIZE,
		.setkey = kmb_ocs_aes_set_key,
		.encrypt = kmb_ocs_aes_ecb_encrypt,
		.decrypt = kmb_ocs_aes_ecb_decrypt
	},
	{
  .base = {
		 .cra_name = "cbc(aes)",
		 .cra_driver_name = "cbc-aes-keembay-ocs",
		 .cra_priority = KMB_OCS_PRIORITY,
		 .cra_flags = CRYPTO_ALG_ASYNC |
					 CRYPTO_ALG_NEED_FALLBACK | //TODO:???
					 CRYPTO_ALG_KERN_DRIVER_ONLY,
		 .cra_blocksize = AES_BLOCK_SIZE,
		 .cra_ctxsize = 0,
		 .cra_alignmask = 0,
		 .cra_module = THIS_MODULE,
  },
		//.init = ocs_aes_cra_init,	//TODO: needed?
		.exit = ocs_aes_cra_exit,
		.min_keysize = AES_MIN_KEY_SIZE,
		.max_keysize = AES_MAX_KEY_SIZE,
		.ivsize = AES_BLOCK_SIZE,
		.setkey = kmb_ocs_aes_set_key,
		.encrypt = kmb_ocs_aes_cbc_encrypt,
		.decrypt = kmb_ocs_aes_cbc_decrypt
	},
	{
  .base = {
		 .cra_name = "ctr(aes)",
		 .cra_driver_name = "ctr-aes-keembay-ocs",
		 .cra_priority = KMB_OCS_PRIORITY,
		 .cra_flags = CRYPTO_ALG_ASYNC |
					 CRYPTO_ALG_NEED_FALLBACK | //TODO:???
					 CRYPTO_ALG_KERN_DRIVER_ONLY,
		 .cra_blocksize = 1,
		 .cra_ctxsize = 0,
		 .cra_alignmask = 0,
		 .cra_module = THIS_MODULE,
  },
		//.init = ocs_aes_cra_init,	//TODO: needed?
		.exit = ocs_aes_cra_exit,
		.min_keysize = AES_MIN_KEY_SIZE,
		.max_keysize = AES_MAX_KEY_SIZE,
		.ivsize = AES_BLOCK_SIZE,
		.setkey = kmb_ocs_aes_set_key,
		.encrypt = kmb_ocs_aes_ctr_encrypt,
		.decrypt = kmb_ocs_aes_ctr_decrypt
	},
	{
  .base = {
		 .cra_name = "cts(cbc(aes))",
		 .cra_driver_name = "cts-aes-keembay-ocs",
		 .cra_priority = KMB_OCS_PRIORITY,
		 .cra_flags = CRYPTO_ALG_ASYNC |
					 CRYPTO_ALG_NEED_FALLBACK | //TODO:???
					 CRYPTO_ALG_KERN_DRIVER_ONLY,
		 .cra_blocksize = AES_BLOCK_SIZE,
		 .cra_ctxsize = 0,
		 .cra_alignmask = 0,
		 .cra_module = THIS_MODULE,
  },
		//.init = ocs_aes_cra_init,	//TODO: needed?
		.exit = ocs_aes_cra_exit,
		.min_keysize = AES_MIN_KEY_SIZE,
		.max_keysize = AES_MAX_KEY_SIZE,
		.ivsize = AES_BLOCK_SIZE,
		.setkey = kmb_ocs_aes_set_key,
		.encrypt = kmb_ocs_aes_cts_encrypt,
		.decrypt = kmb_ocs_aes_cts_decrypt
	}
};

static struct aead_alg algs_aead[] = {
	{
		.base = {
			.cra_name = "gcm(aes)",
			.cra_driver_name = "gcm-aes-keembay-ocs",
			.cra_priority = KMB_OCS_PRIORITY,
			.cra_flags = CRYPTO_ALG_KERN_DRIVER_ONLY,
			.cra_blocksize = 1,
			.cra_ctxsize = 0,
			.cra_alignmask = 0,
			.cra_module = THIS_MODULE,
		},
		//.init = cra_init,	//TODO: needed?
		.exit = ocs_aes_aead_cra_exit,
		.ivsize = GCM_AES_IV_SIZE,
		.maxauthsize = AES_BLOCK_SIZE,
		.setkey = kmb_ocs_aes_aead_set_key,
		.encrypt = kmb_ocs_aes_gcm_encrypt,
		.decrypt = kmb_ocs_aes_gcm_decrypt,
	},
	{
		.base = {
			.cra_name = "ccm(aes)",
			.cra_driver_name = "ccm-aes-keembay-ocs",
			.cra_priority = KMB_OCS_PRIORITY,
			.cra_flags = CRYPTO_ALG_KERN_DRIVER_ONLY,
			.cra_blocksize = 1,
			.cra_ctxsize = 0,
			.cra_alignmask = 0,
			.cra_module = THIS_MODULE,
		},
		//.init = cra_init,	//TODO: needed?
		.exit = ocs_aes_aead_cra_exit,
		.ivsize = AES_BLOCK_SIZE,
		.maxauthsize = AES_BLOCK_SIZE,
		.setkey = kmb_ocs_aes_aead_set_key,
		.encrypt = kmb_ocs_aes_ccm_encrypt,
		.decrypt = kmb_ocs_aes_ccm_decrypt,
	}
};


static int __init register_aes_algs(void)
{
	int ret;

	pr_info("Registering OCS crypto algorithms\n"); //TODO: remove?
	ret = crypto_register_skciphers(algs, ARRAY_SIZE(algs));
	if (ret)
		return ret;
	ret = crypto_register_aeads(algs_aead, ARRAY_SIZE(algs_aead));
	if (ret)
		crypto_unregister_skciphers(algs, ARRAY_SIZE(algs));

	return ret;
}

static void __exit unregister_aes_algs(void)
{
	pr_info("Unregistering OCS crypto algorithms\n"); //TODO: remove?
	crypto_unregister_skciphers(algs, ARRAY_SIZE(algs));
	crypto_unregister_aeads(algs_aead, ARRAY_SIZE(algs_aead));
}

MODULE_DESCRIPTION("Keembay Offload and Crypto Subsystem (OCS) AES Driver");
MODULE_LICENSE("GPL v2");
MODULE_ALIAS_CRYPTO("ecb(aes)");
MODULE_ALIAS_CRYPTO("ecb-aes-keembay-ocs");
MODULE_ALIAS_CRYPTO("cbc(aes)");
MODULE_ALIAS_CRYPTO("cbc-aes-keembay-ocs");
MODULE_ALIAS_CRYPTO("ctr(aes)");
MODULE_ALIAS_CRYPTO("ctr-aes-keembay-ocs");
MODULE_ALIAS_CRYPTO("gcm(aes)");
MODULE_ALIAS_CRYPTO("gcm-aes-keembay-ocs");
MODULE_ALIAS_CRYPTO("ccm(aes)");
MODULE_ALIAS_CRYPTO("ccm-aes-keembay-ocs");
