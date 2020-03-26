// SPDX-License-Identifier: GPL-2.0-only
/*
 * KeemBay OCS HCU Crypto Driver.
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
#include <crypto/engine.h>
#include <crypto/scatterwalk.h>
#include <crypto/algapi.h>
#include <crypto/sha.h>
#include <crypto/sm3.h>
#include <crypto/hash.h>
#include <crypto/internal/hash.h>

#include "thunderbay-ocs-hcu-core.h"
#include "ocs-hcu.h"

#define DRV_NAME "thunderbay-ocs-hcu-driver"

/* Request flags */
#define REQ_FLAGS_HASH_MASK (REQ_FLAGS_SHA_224 | \
			     REQ_FLAGS_SHA_256 | \
			     REQ_FLAGS_SHA_384 | \
			     REQ_FLAGS_SHA_512 | \
			     REQ_FLAGS_SM3)
#define REQ_FLAGS_SHA_224 BIT(0)
#define REQ_FLAGS_SHA_256 BIT(1)
#define REQ_FLAGS_SHA_384 BIT(2)
#define REQ_FLAGS_SHA_512 BIT(3)
#define REQ_FLAGS_SM3 BIT(4)
#define REQ_INTERMEDIATE_DATA BIT(10)
#define REQ_FINAL_DATA BIT(11)
#define REQ_UPDATE BIT(16)
#define REQ_FINAL BIT(17)
#define REQ_FINUP BIT(18)
#define REQ_CMD_MASK (REQ_UPDATE | \
		      REQ_FINAL | \
		      REQ_FINUP)

#define KMB_HCU_BUF_NUM 5
#define KMB_HCU_BUF_SIZE KMB_OCS_HCU_DMA_ALIGNMENT
#define KMB_OCS_HCU_DMA_ALIGNMENT 64
#define KMB_OCS_HCU_ALIGN_MASK 0xf

/* Transform context. */
struct ocs_hcu_ctx {
	struct crypto_engine_ctx engine_ctx;
	struct ocs_hcu_dev *hcu_dev;
};

/* Context for the request. */
struct ocs_hcu_rctx {
	u32 flags;
	size_t blk_sz;

	/* Head of the unhandled scatterlist entries containing data. */
	struct scatterlist *sg;
	dma_addr_t dma_ll_phys;
	/* OCS DMA linked list head. */
	struct ocs_hcu_dma_desc *dma_list_head;
	/* OCS DMA linked list tail. */
	struct ocs_hcu_dma_desc *dma_list_tail;
	/* The size of the allocated buffer to contain the DMA linked list. */
	size_t dma_list_size;

	struct ocs_hcu_dev *hcu_dev;
	struct ocs_hcu_idata_desc idata;
	u32 algo;
	u32 dig_sz;

	/* Total data in the SG list at any time. */
	unsigned int sg_data_total;
	/* Offset into the data of an individual SG node. */
	unsigned int sg_data_offset;
	/* The amount of data in bytes in each buffer. */
	unsigned int buf_cnt[KMB_HCU_BUF_NUM];
	/* The statig length of each buffer. */
	unsigned int buf_len;
	/* The index indicating the next free buffer. */
	unsigned int buf_free_idx;
	/* The index indicating the buffer containing the first data. */
	unsigned int buf_data_idx;
	/* The total amount of data inside the buffers */
	unsigned int buf_total;

	u8 buffer[KMB_HCU_BUF_NUM][KMB_HCU_BUF_SIZE] __aligned(sizeof(u64));
};

/* Driver data. */
struct ocs_hcu_drv {
	struct list_head dev_list;
	spinlock_t lock;
};

static struct ocs_hcu_drv ocs_hcu = {
	.dev_list = LIST_HEAD_INIT(ocs_hcu.dev_list),
	.lock = __SPIN_LOCK_UNLOCKED(ocs_hcu.lock),
};

static inline unsigned int kmb_get_total_data(struct ocs_hcu_rctx *rctx)
{
	unsigned int total = rctx->sg_data_total;
	int i;

	for (i = 0; i < KMB_HCU_BUF_NUM; i++)
		total += rctx->buf_cnt[i];
	return total;
}

static void kmb_ocs_hcu_copy_sg(struct ocs_hcu_rctx *rctx,
				void *buf, size_t count)
{
	scatterwalk_map_and_copy(buf, rctx->sg, rctx->sg_data_offset, count, 0);

	rctx->sg_data_offset += count;
	rctx->sg_data_total -= count;
	rctx->buf_cnt[rctx->buf_free_idx] += count;
	rctx->buf_total += count;

	if (rctx->sg_data_offset == rctx->sg->length) {
		rctx->sg = sg_next(rctx->sg);
		rctx->sg_data_offset = 0;
		if (!rctx->sg)
			rctx->sg_data_total = 0;
	}
}

/* As the SG is copied byte-for-byte into the buffers, this may lead to the
 * data source address not being aligned for the HCU DMA engine. This function
 * will use the next free buffer to copy data into an aligned location and thus
 * aligning the source address in the SG list.
 * Note that it is assumed the SG data source address is aligned.
 */
static int kmb_ocs_hcu_align_sg(struct ocs_hcu_rctx *rctx)
{
	size_t count;
	u32 src_alignmen;

	if (!rctx->sg)
		return 0;

	if (rctx->buf_free_idx >= KMB_HCU_BUF_NUM) {
		dev_err(rctx->hcu_dev->dev,
			"Free buffer index out of bounds.\n");
		return -EINVAL;
	}

	src_alignmen = sg_phys(rctx->sg) & KMB_OCS_HCU_ALIGN_MASK;

	if (rctx->sg_data_offset & KMB_OCS_HCU_ALIGN_MASK) {
		count = min((KMB_OCS_HCU_ALIGN_MASK + 1) -
			    (rctx->sg_data_offset & KMB_OCS_HCU_ALIGN_MASK),
			    rctx->sg->length - rctx->sg_data_offset);

		if (src_alignmen)
			count += (KMB_OCS_HCU_ALIGN_MASK + 1) - src_alignmen;

		kmb_ocs_hcu_copy_sg(rctx, rctx->buffer[rctx->buf_free_idx],
				    count);
	}
	/* Always increase the buffer number. */
	rctx->buf_free_idx = (rctx->buf_free_idx + 1) % KMB_HCU_BUF_NUM;
	return 0;
}

static int kmb_ocs_hcu_append_sg(struct ocs_hcu_rctx *rctx)
{
	unsigned int count;

	if (!rctx->sg)
		return 0;

	if (unlikely(rctx->buf_free_idx >= KMB_HCU_BUF_NUM)) {
		dev_err(rctx->hcu_dev->dev,
			"Free buffer index out of bounds.\n");
		return -EINVAL;
	}

	/* Only ever get a DMA block size in the buffers. */
	while ((rctx->buf_total < rctx->blk_sz) && rctx->sg_data_total) {
		/* Determine the maximum data available to copy from the node.
		 * Minimum of the length left in the sg node, or the total data
		 * in the request.
		 * Then ensure that the buffer has the space available, so
		 * determine the minimum of the previous minimum with the
		 * remaining buffer size.
		 */
		count = min(rctx->sg->length - rctx->sg_data_offset,
			       rctx->sg_data_total);
		count = min(count, rctx->buf_len -
				rctx->buf_cnt[rctx->buf_free_idx]);

		if (count <= 0) {
			if ((rctx->sg->length == 0) && !sg_is_last(rctx->sg)) {
				rctx->sg = sg_next(rctx->sg);
				continue;
			} else
				break;
		}
		kmb_ocs_hcu_copy_sg(rctx, rctx->buffer[rctx->buf_free_idx] +
				    rctx->buf_cnt[rctx->buf_free_idx], count);
		if (rctx->buf_cnt[rctx->buf_free_idx] >= rctx->buf_len)
			rctx->buf_free_idx = (rctx->buf_free_idx + 1) %
				KMB_HCU_BUF_NUM;
	}
	return 0;
}

static inline void kmb_ocs_hcu_clear_buffer(struct ocs_hcu_rctx *rctx)
{
	int i = 0;

	for (i = 0; i < KMB_HCU_BUF_NUM; i++)
		rctx->buf_cnt[i] = 0;

	rctx->buf_total = 0;
}

static struct ocs_hcu_dev *kmb_ocs_hcu_find_dev(struct ahash_request *req)
{
	struct crypto_ahash *tfm = crypto_ahash_reqtfm(req);
	struct ocs_hcu_ctx *tctx = crypto_ahash_ctx(tfm);
	struct ocs_hcu_dev *hcu_dev = NULL, *tmp;

	spin_lock_bh(&ocs_hcu.lock);
	if (!tctx->hcu_dev) {
		list_for_each_entry(tmp, &ocs_hcu.dev_list, list) {
			hcu_dev = tmp;
			break;
		}
		tctx->hcu_dev = hcu_dev;
	} else {
		hcu_dev = tctx->hcu_dev;
	}

	spin_unlock_bh(&ocs_hcu.lock);
	return hcu_dev;
}

static void kmb_ocs_free_dma_list(struct ocs_hcu_rctx *rctx,
				  size_t dma_list_size)
{
	struct ocs_hcu_dev *hcu_dev = rctx->hcu_dev;
	struct device *dev = hcu_dev->dev;
	struct ocs_hcu_dma_desc *dma_tail = rctx->dma_list_tail;
	int i, n = dma_list_size /
		    sizeof(*rctx->dma_list_head);

	/* Check for a 0 size list. If so, nothing allocated. */
	if (n == 0)
		return;

	/* Iterate over successfully mapped sg addresses. */
	for (i = 0; i < n; i++) {
		if (!dma_mapping_error(dev, dma_tail->src_adr))
			dma_unmap_single(dev, dma_tail->src_adr,
					 dma_tail->src_len,
					 DMA_TO_DEVICE);
		dma_tail--;
	}

	if (!dma_mapping_error(dev, rctx->dma_ll_phys))
		dma_unmap_single(dev, rctx->dma_ll_phys,
				 dma_list_size, DMA_TO_DEVICE);

	kfree(rctx->dma_list_head);
	rctx->dma_list_head = 0;
	rctx->dma_list_tail = 0;
	rctx->dma_ll_phys = 0;
}

static int kmb_ocs_add_dma_tail(struct ocs_hcu_rctx *rctx,
				dma_addr_t addr, size_t len)
{
	if (addr & KMB_OCS_HCU_ALIGN_MASK || addr > OCS_HCU_DMA_MAX_ADDR_MASK)
		return -EINVAL;

	if (!len)
		return 0;

	rctx->dma_list_tail->src_adr = (u32)addr;
	rctx->dma_list_tail->src_len = (u32)len;
	rctx->dma_list_tail->ll_flags = 0;
	rctx->dma_list_tail->nxt_desc = rctx->dma_ll_phys +
					(virt_to_phys(rctx->dma_list_tail) -
					virt_to_phys(rctx->dma_list_head)) +
					sizeof(*rctx->dma_list_tail);
	rctx->dma_list_tail++;

	return 0;
}

static void kmb_ocs_terminate_dma_tail(struct ocs_hcu_rctx *rctx)
{
	if (rctx->dma_list_head <= rctx->dma_list_tail)
		rctx->dma_list_tail--;


	rctx->dma_list_tail->nxt_desc = 0;
	rctx->dma_list_tail->ll_flags = OCS_LL_DMA_FLAG_TERMINATE;
}

/* This function will always have a total DMA size aligned to 64B. */
static int kmb_ocs_init_dma_list(struct ahash_request *req)
{
	struct ocs_hcu_rctx *rctx = ahash_request_ctx(req);
	struct ocs_hcu_dev *hcu_dev = kmb_ocs_hcu_find_dev(req);
	struct scatterlist *sg = req->src;
	dma_addr_t data_addr = 0;
	struct device *dev = hcu_dev->dev;
	unsigned int remainder = 0;
	size_t sz, count;
	unsigned int total = kmb_get_total_data(rctx);
	int i, n, nents = sg_nents(rctx->sg);


	if (!total)
		return 0;

	n = nents;
	/* If there is data in any of the buffers, increase the number of
	 * individual nodes in the linked list to be allocated.
	 */
	for (i = 0; i < KMB_HCU_BUF_NUM; i++)
		n = rctx->buf_cnt[i] > 0 ? n + 1 : n;

	/* Size of the total number of descriptors to allocate. */
	sz = sizeof(*rctx->dma_list_head) * n;
	rctx->dma_list_head = (struct ocs_hcu_dma_desc *)
		kzalloc(sz, GFP_KERNEL);
	if (unlikely(!rctx->dma_list_head))
		return -ENOMEM;

	rctx->dma_list_tail = rctx->dma_list_head;
	rctx->dma_list_size = sz;
	/* For descriptor fetch, get the DMA address. */
	rctx->dma_ll_phys = dma_map_single(dev, rctx->dma_list_head, sz,
						DMA_TO_DEVICE);


	if (unlikely(dma_mapping_error(dev, rctx->dma_ll_phys)))
		goto cleanup;

	/* If this is not a final DMA (terminated DMA), the data passed to the
	 * HCU must be algigned to the block size.
	 */
	if (!(rctx->flags & REQ_FINAL_DATA))
		remainder = (total) % rctx->blk_sz;

	dma_sync_single_for_cpu(dev, rctx->dma_ll_phys, sz, DMA_TO_DEVICE);
	/* DMA map and get each SG address and length. */
	/* First, if any of the buf_cnt > 0, create a node. */
	for (i = rctx->buf_data_idx;
			i < (KMB_HCU_BUF_NUM + rctx->buf_data_idx); i++) {
		if (rctx->buf_cnt[i % KMB_HCU_BUF_NUM] == 0)
			continue;
		data_addr = dma_map_single(dev,
				rctx->buffer[i % KMB_HCU_BUF_NUM],
				rctx->buf_cnt[i % KMB_HCU_BUF_NUM],
				DMA_TO_DEVICE);
		if (unlikely(dma_mapping_error(dev, data_addr)))
			goto cleanup;

		if (kmb_ocs_add_dma_tail(rctx, data_addr,
			rctx->buf_cnt[i % KMB_HCU_BUF_NUM]))
			goto cleanup;
	}

	for_each_sg(rctx->sg, sg, nents, i) {
		count = min(rctx->sg_data_total - remainder,
			    sg->length - rctx->sg_data_offset);

		/* Do not create a zero length DMA descriptor. Check in case of
		 * zero length SG node.
		 */
		if (count > 0) {
			data_addr = dma_map_single(dev, sg_virt(sg),
						   sg->length,
						   DMA_TO_DEVICE);
			if (unlikely(dma_mapping_error(dev, data_addr)))
				goto cleanup;

			if (kmb_ocs_add_dma_tail(rctx, data_addr +
						rctx->sg_data_offset,
						count))
				goto cleanup;

			rctx->sg_data_offset += count;
			rctx->sg_data_total -= count;
		}

		if (rctx->sg_data_offset == sg->length) {
			rctx->sg_data_offset = 0;
			rctx->sg = sg_next(rctx->sg);
			if (!rctx->sg)
				break;
		}
	}

	rctx->buf_data_idx = rctx->buf_free_idx;

	kmb_ocs_terminate_dma_tail(rctx);

	dma_sync_single_for_device(dev, rctx->dma_ll_phys, sz, DMA_TO_DEVICE);

	return 0;
cleanup:
	dev_err(dev, "Failed to map DMA buffers.\n");
	kmb_ocs_free_dma_list(rctx, sz);

	return -ENOMEM;
}

static int kmb_ocs_hcu_set_hw(struct ocs_hcu_dev *hcu_dev, int algo)
{
	struct ocs_hcu_rctx *rctx = ahash_request_ctx(hcu_dev->req);
	int rc = 0;

	/* Configure the hardware for the current request. */
	rc = ocs_hcu_hw_cfg(hcu_dev, algo);

	if (rc)
		return rc;

	if (rctx->flags & REQ_INTERMEDIATE_DATA) {
		ocs_hcu_set_intermediate_data(hcu_dev, &rctx->idata, algo);
		rctx->flags &= ~REQ_INTERMEDIATE_DATA;
	}
	return 0;
}

static int kmb_ocs_hcu_tx_dma(struct ocs_hcu_dev *hcu_dev, int algo)
{
	struct ocs_hcu_rctx *rctx = ahash_request_ctx(hcu_dev->req);
	bool term = (rctx->flags & REQ_FINAL_DATA) == REQ_FINAL_DATA;
	int rc;

	if (!rctx->dma_ll_phys)
		return 0;
	/* Configure the hardware for the current request. */
	rc = kmb_ocs_hcu_set_hw(hcu_dev, algo);

	if (!rc) {
		/* Start the DMA engine with the descriptor address stored. */
		ocs_hcu_ll_dma_start(hcu_dev, rctx->dma_ll_phys, term);
		if (!term)
			rctx->flags |= REQ_INTERMEDIATE_DATA;
	}
	return rc;
}

static int kmb_ocs_hcu_tx(struct ocs_hcu_dev *hcu_dev, int algo)
{
	int rc = 0;

	/* If a DMA linked list has been created, handle it. */
	rc = kmb_ocs_hcu_tx_dma(hcu_dev, algo);
	return rc;
}

static void kmb_ocs_hcu_complete_request(struct ocs_hcu_dev *hcu_dev, int error)
{
	struct ocs_hcu_rctx *rctx = ahash_request_ctx(hcu_dev->req);

	if (error == 0 && !(rctx->flags & REQ_UPDATE)) {
		if (hcu_dev->req->result) {
			memcpy(hcu_dev->req->result,
			       rctx->idata.digest, rctx->dig_sz);
		}
	}

	if (rctx->flags & REQ_FINUP || rctx->flags & REQ_FINAL)
		ocs_hcu_hw_disable(hcu_dev);

	crypto_finalize_hash_request(hcu_dev->engine, hcu_dev->req, error);
}

static void kmb_ocs_hcu_finish_request(struct ocs_hcu_dev *hcu_dev, int error)
{
	struct ocs_hcu_rctx *rctx = ahash_request_ctx(hcu_dev->req);

	/* Buffers are now clear for next tx. */
	kmb_ocs_hcu_clear_buffer(rctx);

	/* Gets the intermediate data (including digest). */
	ocs_hcu_finish_req(hcu_dev, rctx->algo, &rctx->idata, error);

	/* Always copy over into result as well. */
	kmb_ocs_hcu_complete_request(hcu_dev, error);
}

static int kmb_ocs_hcu_handle_queue(struct ahash_request *req)
{
	struct ocs_hcu_dev *hcu_dev = kmb_ocs_hcu_find_dev(req);

	return crypto_transfer_hash_request_to_engine(hcu_dev->engine,
						      req);
}

/* If there is data, this function will create a HCU DMA linked list.
 * There are two requirements to starting the DMA engine to ensure completion:
 *    1) The data address alignment is is to 8B
 *    2) The data size is aligned to the algorithm's block size
 * With these constraints, the prepare function will create the linked lists
 * for the DMA engine if the data size is above the block size.
 * Any residual/unaligned data will remain in a buffer until the next
 * update/final/finup.
 *
 * Exception:
 * If the transfer is to be terminated (Getting the final hash) a DMA can
 * be performed if the HCU is programmed for receiving the final data.
 */
static int kmb_ocs_hcu_prepare_request(struct crypto_engine *engine, void *areq)
{
	struct ahash_request *req = container_of(areq, struct ahash_request,
						 base);
	struct ocs_hcu_rctx *rctx = ahash_request_ctx(req);
	struct ocs_hcu_dev *hcu_dev = kmb_ocs_hcu_find_dev(req);
	int rc = 0;
	unsigned int total = kmb_get_total_data(rctx);

	hcu_dev->req = req;

	if (total == 0)
		return 0;

	/* If aligned or final data, create linked list. */
	if (rctx->flags & REQ_FINAL_DATA ||
	    (total % rctx->blk_sz) == 0) {
		/* Ensure that the SG offset is aligned. */
		rc = kmb_ocs_hcu_align_sg(rctx);
		return rc ?: kmb_ocs_init_dma_list(req);
	}
	/* On updates, prepare is only called if the data is above the
	 * the algorithm's block size. if the data is unaligned, the DMA
	 * will be programmed to handle as much data as possible for the
	 * request, but some data will be left in the SG list that will
	 * be copied to the buffers in the kmb_ocs_hcu_unprepare_request
	 * function that is called on successful DMA completion.
	 */
	if (rctx->flags & REQ_UPDATE) {
		/* Cannot directly create aligned linked lists.
		 * Fill and align buffers.
		 */
		/* Ensure the buffers are full up to 1 block size. */
		rc = kmb_ocs_hcu_append_sg(rctx);
		/* Ensure that the SG offset is aligned for the DMA engine. */
		rc |= kmb_ocs_hcu_align_sg(rctx);
		/* Create DMA linked list. */
		return rc ?: kmb_ocs_init_dma_list(req);
		/* Data may remain in the sg, ensure copy after DMA */
		/* in unprepare. */
	}
	return 0;
}

static int kmb_ocs_hcu_do_one_request(struct crypto_engine *engine, void *areq)
{
	struct ahash_request *req = container_of(areq, struct ahash_request,
						 base);
	struct ocs_hcu_rctx *rctx = ahash_request_ctx(req);
	struct ocs_hcu_dev *hcu_dev = kmb_ocs_hcu_find_dev(req);
	int rc = 0;

	if (rctx->flags & REQ_UPDATE || rctx->flags & REQ_FINUP) {
		/* Perform transfer. */
		rc = kmb_ocs_hcu_tx(hcu_dev, rctx->algo);
	} else if (rctx->flags & REQ_FINAL) {
		/* If the REQ_FINAL_DATA flag is set the transform is terminated
		 * or there is data to perform a final hash.
		 * Otherwise terminate the transform to obtain the final hash.
		 */
		if ((rctx->flags & REQ_FINAL_DATA) != REQ_FINAL_DATA) {
			rc = kmb_ocs_hcu_set_hw(hcu_dev, rctx->algo);
			if (rc)
				return rc;
			ocs_hcu_tx_data_done(hcu_dev);
		} else {
			rc = kmb_ocs_hcu_tx(hcu_dev, rctx->algo);
		}
	}
	return rc;
}

static int kmb_ocs_hcu_unprepare_request(struct crypto_engine *engine,
					 void *areq)
{
	struct ahash_request *req = container_of(areq, struct ahash_request,
						 base);
	struct ocs_hcu_rctx *rctx = ahash_request_ctx(req);
	int rc = 0;

	if (rctx->dma_ll_phys)
		kmb_ocs_free_dma_list(rctx, rctx->dma_list_size);

	/* Copy residual data which was not aligned to init buff for
	 * next transfer.
	 */
	rc = kmb_ocs_hcu_append_sg(rctx);

	return rc;
}

static int kmb_ocs_hcu_init(struct ahash_request *req)
{
	struct crypto_ahash *tfm = crypto_ahash_reqtfm(req);
	struct ocs_hcu_rctx *rctx = ahash_request_ctx(req);
	struct ocs_hcu_dev *hcu_dev = kmb_ocs_hcu_find_dev(req);

	rctx->flags = 0;
	rctx->buf_len = KMB_HCU_BUF_SIZE;
	rctx->hcu_dev = hcu_dev;
	rctx->dig_sz = crypto_ahash_digestsize(tfm);

	switch (rctx->dig_sz) {
	case SHA224_DIGEST_SIZE:
		rctx->flags |= REQ_FLAGS_SHA_224;
		rctx->blk_sz = SHA224_BLOCK_SIZE;
		rctx->algo = OCS_HCU_ALGO_SHA224;
		break;
	case SHA256_DIGEST_SIZE:
		rctx->flags |= REQ_FLAGS_SHA_256;
		rctx->blk_sz = SHA256_BLOCK_SIZE;
		rctx->algo = OCS_HCU_ALGO_SHA256;
		break;
	case SHA384_DIGEST_SIZE:
		rctx->flags |= REQ_FLAGS_SHA_384;
		rctx->blk_sz = SHA384_BLOCK_SIZE;
		rctx->algo = OCS_HCU_ALGO_SHA384;
		break;
	case SHA512_DIGEST_SIZE:
		rctx->flags |= REQ_FLAGS_SHA_512;
		rctx->blk_sz = SHA512_BLOCK_SIZE;
		rctx->algo = OCS_HCU_ALGO_SHA512;
		break;
	/* TODO - Implement SM3 differentiation. */
	/* TODO - Implement HMAC algos. */
	default:
		return -EINVAL;
	//	break;
	}

	rctx->dma_ll_phys = 0;
	rctx->dma_list_tail = 0;
	rctx->dma_list_head = 0;
	rctx->dma_list_size = 0;
	/* Clear the total transfer size. */
	rctx->sg_data_total = 0;
	rctx->sg_data_offset = 0;
	rctx->buf_free_idx = 0;
	rctx->buf_data_idx = 0;

	kmb_ocs_hcu_clear_buffer(rctx);

	/* Clear the intermediate data. */
	memset((void *)&rctx->idata, 0, sizeof(rctx->idata));

	ocs_hcu_hw_init(hcu_dev);

	return 0;
}

static int kmb_ocs_hcu_update(struct ahash_request *req)
{
	struct ocs_hcu_rctx *rctx = ahash_request_ctx(req);

	if (!req->nbytes)
		return 0;

	rctx->sg_data_offset = 0;
	rctx->sg_data_total += req->nbytes;
	rctx->sg = req->src;
	rctx->flags &= ~REQ_CMD_MASK;
	rctx->flags |= REQ_UPDATE;

	if (rctx->sg_data_total <= (rctx->blk_sz - rctx->buf_total))
		return kmb_ocs_hcu_append_sg(rctx);

	return kmb_ocs_hcu_handle_queue(req);
}

static int kmb_ocs_hcu_final(struct ahash_request *req)
{
	struct ocs_hcu_rctx *rctx = ahash_request_ctx(req);

	rctx->flags &= ~REQ_CMD_MASK;
	rctx->flags |= REQ_FINAL;

	if (rctx->buf_cnt[rctx->buf_data_idx])
		rctx->flags |= REQ_FINAL_DATA;
	return kmb_ocs_hcu_handle_queue(req);
}

static int kmb_ocs_hcu_finup(struct ahash_request *req)
{
	struct ocs_hcu_rctx *rctx = ahash_request_ctx(req);

	rctx->sg_data_total += req->nbytes;
	rctx->sg = req->src;

	rctx->flags &= ~REQ_CMD_MASK;
	/* If there is data to process, do Final update. If not, FINAL. */
	if (!req->nbytes) {
		rctx->flags |= REQ_FINAL;
	} else {
		rctx->flags |= REQ_FINAL_DATA;
		rctx->flags |= REQ_FINUP;
	}
	return kmb_ocs_hcu_handle_queue(req);
}

static int kmb_ocs_hcu_digest(struct ahash_request *req)
{
	int rc = 0;

	rc = kmb_ocs_hcu_init(req);
	if (rc)
		return rc;

	rc = kmb_ocs_hcu_finup(req);

	return rc;
}

static int kmb_ocs_hcu_export(struct ahash_request *req, void *out)
{
	struct ocs_hcu_rctx *rctx = ahash_request_ctx(req);

	/* Intermediate data is always stored and applied per request. */
	memcpy(out, rctx, sizeof(*rctx));

	return 0;
}

static int kmb_ocs_hcu_import(struct ahash_request *req, const void *in)
{
	struct ocs_hcu_rctx *rctx = ahash_request_ctx(req);

	/* Intermediate data is always stored and applied per request. */
	memcpy(rctx, in, sizeof(*rctx));

	return 0;
}

static int kmb_ocs_hcu_sha_cra_init(struct crypto_tfm *tfm)
{
	struct ocs_hcu_ctx *ctx = crypto_tfm_ctx(tfm);

	crypto_ahash_set_reqsize(__crypto_ahash_cast(tfm),
				 sizeof(struct ocs_hcu_rctx));

	ctx->engine_ctx.op.do_one_request = kmb_ocs_hcu_do_one_request;
	ctx->engine_ctx.op.prepare_request = kmb_ocs_hcu_prepare_request;
	ctx->engine_ctx.op.unprepare_request = kmb_ocs_hcu_unprepare_request;

	return 0;
}

static struct ahash_alg ocs_hcu_algs[] = {
{
	.init		= kmb_ocs_hcu_init,
	.update		= kmb_ocs_hcu_update,
	.final		= kmb_ocs_hcu_final,
	.finup		= kmb_ocs_hcu_finup,
	.digest		= kmb_ocs_hcu_digest,
	.export		= kmb_ocs_hcu_export,
	.import		= kmb_ocs_hcu_import,
	.halg = {
		.digestsize	= SHA224_DIGEST_SIZE,
		.statesize	= sizeof(struct ocs_hcu_rctx),
		.base	= {
			.cra_name		= "sha224",
			.cra_driver_name	= "sha244-keembay-ocs",
			.cra_priority		= 255,
			.cra_flags		= CRYPTO_ALG_ASYNC,
			.cra_blocksize		= SHA224_BLOCK_SIZE,
			.cra_ctxsize		= sizeof(struct ocs_hcu_ctx),
			.cra_alignmask		= KMB_OCS_HCU_ALIGN_MASK,
			.cra_module		= THIS_MODULE,
			.cra_init		= kmb_ocs_hcu_sha_cra_init,
		}
	}
},
{
	.init		= kmb_ocs_hcu_init,
	.update		= kmb_ocs_hcu_update,
	.final		= kmb_ocs_hcu_final,
	.finup		= kmb_ocs_hcu_finup,
	.digest		= kmb_ocs_hcu_digest,
	.export		= kmb_ocs_hcu_export,
	.import		= kmb_ocs_hcu_import,
	.halg = {
		.digestsize	= SHA256_DIGEST_SIZE,
		.statesize	= sizeof(struct ocs_hcu_rctx),
		.base	= {
			.cra_name		= "sha256",
			.cra_driver_name	= "sha256-keembay-ocs",
			.cra_priority		= 255,
			.cra_flags		= CRYPTO_ALG_ASYNC,
			.cra_blocksize		= SHA256_BLOCK_SIZE,
			.cra_ctxsize		= sizeof(struct ocs_hcu_ctx),
			.cra_alignmask		= KMB_OCS_HCU_ALIGN_MASK,
			.cra_module		= THIS_MODULE,
			.cra_init		= kmb_ocs_hcu_sha_cra_init,
		}
	}
},
{
	.init		= kmb_ocs_hcu_init,
	.update		= kmb_ocs_hcu_update,
	.final		= kmb_ocs_hcu_final,
	.finup		= kmb_ocs_hcu_finup,
	.digest		= kmb_ocs_hcu_digest,
	.export		= kmb_ocs_hcu_export,
	.import		= kmb_ocs_hcu_import,
	.halg = {
		.digestsize	= SHA384_DIGEST_SIZE,
		.statesize	= sizeof(struct ocs_hcu_rctx),
		.base	= {
			.cra_name		= "sha384",
			.cra_driver_name	= "sha384-keembay-ocs",
			.cra_priority		= 255,
			.cra_flags		= CRYPTO_ALG_ASYNC,
			.cra_blocksize		= SHA384_BLOCK_SIZE,
			.cra_ctxsize		= sizeof(struct ocs_hcu_ctx),
			.cra_alignmask		= KMB_OCS_HCU_ALIGN_MASK,
			.cra_module		= THIS_MODULE,
			.cra_init		= kmb_ocs_hcu_sha_cra_init,
		}
	}
},
{
	.init		= kmb_ocs_hcu_init,
	.update		= kmb_ocs_hcu_update,
	.final		= kmb_ocs_hcu_final,
	.finup		= kmb_ocs_hcu_finup,
	.digest		= kmb_ocs_hcu_digest,
	.export		= kmb_ocs_hcu_export,
	.import		= kmb_ocs_hcu_import,
	.halg = {
		.digestsize	= SHA512_DIGEST_SIZE,
		.statesize	= sizeof(struct ocs_hcu_rctx),
		.base	= {
			.cra_name		= "sha512",
			.cra_driver_name	= "sha512-keembay-ocs",
			.cra_priority		= 255,
			.cra_flags		= CRYPTO_ALG_ASYNC,
			.cra_blocksize		= SHA512_BLOCK_SIZE,
			.cra_ctxsize		= sizeof(struct ocs_hcu_ctx),
			.cra_alignmask		= KMB_OCS_HCU_ALIGN_MASK,
			.cra_module		= THIS_MODULE,
			.cra_init		= kmb_ocs_hcu_sha_cra_init,
		}
	}
}
};

static irqreturn_t kmb_ocs_hcu_irq_thread(int irq, void *dev_id)
{
	struct ocs_hcu_dev *hcu_dev = (struct ocs_hcu_dev *)dev_id;
	int rc = 0;

	if (hcu_dev->flags & HCU_FLAGS_HCU_ERROR_MASK)
		rc = -EIO;

	kmb_ocs_hcu_finish_request(hcu_dev, rc);
	return IRQ_HANDLED;
}

static int kmb_ocs_hcu_unregister_algs(struct ocs_hcu_dev *hcu_dev)
{
	int i = 0;
	int rc = 0;
	int ret = 0;

	for (; i < ARRAY_SIZE(ocs_hcu_algs); i++)
		crypto_unregister_ahash(&ocs_hcu_algs[i]);

	return ret;
}

static int kmb_ocs_hcu_register_algs(struct ocs_hcu_dev *hcu_dev)
{
	int i, j;
	int rc = 0;
	int err_rc = 0;

	for (i = 0; i < ARRAY_SIZE(ocs_hcu_algs); i++) {
		rc = crypto_register_ahash(&ocs_hcu_algs[i]);
		if (rc)
			goto cleanup;
		dev_info(hcu_dev->dev, "Registered %s.\n",
			ocs_hcu_algs[i].halg.base.cra_name);
	}
	return 0;
cleanup:
	dev_err(hcu_dev->dev, "Failed to register algo.\n");
	for (j = 0; j < i; j++)
		crypto_unregister_ahash(&ocs_hcu_algs[i]);
	return rc;
}


/* Device tree driver match. */
static const struct of_device_id kmb_ocs_hcu_of_match[] = {
	{
		.compatible = "intel,keembay-ocs-hcu",
	},
	{}
};

static int kmb_ocs_hcu_remove(struct platform_device *pdev)
{
	struct ocs_hcu_dev *hcu_dev;

	hcu_dev = platform_get_drvdata(pdev);
	if (!hcu_dev)
		return -ENODEV;

	kmb_ocs_hcu_unregister_algs(hcu_dev);

	ocs_hcu_hw_disable(hcu_dev);

	spin_lock(&ocs_hcu.lock);
	list_del(&hcu_dev->list);
	spin_unlock(&ocs_hcu.lock);

	crypto_engine_exit(hcu_dev->engine);

	return 0;
}

static int kmb_ocs_hcu_probe(struct platform_device *pdev)
{
	int rc;
	struct device *dev = &pdev->dev;
	struct resource *hcu_mem;
	struct ocs_hcu_dev *hcu_dev;

	hcu_dev = devm_kzalloc(dev, sizeof(*hcu_dev),
				       GFP_KERNEL);
	if (!hcu_dev) {
		dev_err(dev, "Allocation of device struct failed.\n");
		return -ENOMEM;
	}

	hcu_dev->dev = dev;

	platform_set_drvdata(pdev, hcu_dev);
	rc = dma_set_mask_and_coherent(&pdev->dev, DMA_BIT_MASK(64));

	if (rc)
		return rc;

	INIT_LIST_HEAD(&hcu_dev->list);

	/* Get the memory address and remap. */
	hcu_mem = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	if (!hcu_mem) {
		dev_err(dev, "Could not retrieve io mem resource.\n");
		rc = -ENODEV;
		goto list_del;
	}

	hcu_dev->io_base = devm_ioremap_resource(dev, hcu_mem);
	if (IS_ERR(hcu_dev->io_base)) {
		dev_err(dev, "Could not io-remap mem resource.\n");
		rc = PTR_ERR(hcu_dev->io_base);
		goto list_del;
	}

	/* Get and request IRQ. */
	hcu_dev->irq = platform_get_irq(pdev, 0);
	if (hcu_dev->irq < 0) {
		dev_err(dev, "Could not retrieve IRQ.\n");
		rc = hcu_dev->irq;
		goto list_del;
	}

	rc = devm_request_threaded_irq(&pdev->dev, hcu_dev->irq,
				       ocs_hcu_irq_handler,
				       kmb_ocs_hcu_irq_thread,
				       0, "keembay-ocs-hcu", hcu_dev);
	if (rc < 0) {
		dev_err(dev, "Could not request IRQ.\n");
		goto list_del;
	}

	spin_lock(&ocs_hcu.lock);
	list_add_tail(&hcu_dev->list, &ocs_hcu.dev_list);
	spin_unlock(&ocs_hcu.lock);

	/* Initialize crypto engine */
	hcu_dev->engine = crypto_engine_alloc_init(dev, 1);
	if (!hcu_dev->engine)
		goto list_del;

	rc = crypto_engine_start(hcu_dev->engine);
	if (rc) {
		dev_err(dev, "Could not start engine.\n");
		goto cleanup;
	}
//ocs_wrapper_init(0x1700000000);

#if 0  //TODO: Enable when clk is available on THB dtb
	/* TODO - is this needed? Clock not changeable. */
	hcu_dev->clk = devm_clk_get(dev, NULL);
	if (IS_ERR(hcu_dev->clk)) {
		dev_err(dev, "Could not get clock.\n");
		rc = PTR_ERR(hcu_dev->clk);
		goto cleanup;
	}

	/* Security infrastructure guarantees OCS clock is enabled. */
	dev_info(&pdev->dev, "OCS clk rate %luHz\n",
			clk_get_rate(hcu_dev->clk));
#endif

	rc = kmb_ocs_hcu_register_algs(hcu_dev);
	if (rc) {
		dev_err(dev, "Could not register algorithms.\n");
		goto cleanup;
	}
	return 0;
cleanup:
	crypto_engine_exit(hcu_dev->engine);
list_del:
	spin_lock(&ocs_hcu.lock);
	list_del(&hcu_dev->list);
	spin_unlock(&ocs_hcu.lock);
	return rc;

}

/* The OCS driver is a platform device. */
static struct platform_driver kmb_ocs_hcu_driver = {
	.probe = kmb_ocs_hcu_probe,
	.remove = kmb_ocs_hcu_remove,
	.driver = {
			.name = DRV_NAME,
			.of_match_table = kmb_ocs_hcu_of_match,
		},
};

module_platform_driver(kmb_ocs_hcu_driver);

MODULE_LICENSE("GPL v2");
