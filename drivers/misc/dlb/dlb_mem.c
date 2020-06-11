// SPDX-License-Identifier: GPL-2.0-only
/* Copyright(c) 2017-2020 Intel Corporation */

#include "dlb_main.h"
#include "dlb_mem.h"

void dlb_release_domain_memory(struct dlb_dev *dev, u32 domain_id)
{
	struct dlb_port_memory *port_mem;
	struct dlb_domain_dev *domain;
	int i;

	domain = &dev->sched_domains[domain_id];

	dev_dbg(dev->dlb_device,
		"Releasing pages pinned for domain %d\n", domain_id);

	for (i = 0; i < DLB_MAX_NUM_LDB_PORTS; i++) {
		port_mem = &dev->ldb_port_mem[i];

		if (port_mem->domain_id == domain_id && port_mem->valid) {
			dma_free_coherent(&dev->pdev->dev,
					  DLB_LDB_CQ_MAX_SIZE,
					  port_mem->cq_base,
					  port_mem->cq_dma_base);

			dma_free_coherent(&dev->pdev->dev,
					  PAGE_SIZE,
					  port_mem->pc_base,
					  port_mem->pc_dma_base);

			port_mem->valid = false;
		}
	}

	for (i = 0; i < DLB_MAX_NUM_DIR_PORTS; i++) {
		port_mem = &dev->dir_port_mem[i];

		if (port_mem->domain_id == domain_id && port_mem->valid) {
			dma_free_coherent(&dev->pdev->dev,
					  DLB_DIR_CQ_MAX_SIZE,
					  port_mem->cq_base,
					  port_mem->cq_dma_base);

			dma_free_coherent(&dev->pdev->dev,
					  PAGE_SIZE,
					  port_mem->pc_base,
					  port_mem->pc_dma_base);

			port_mem->valid = false;
		}
	}
}

void dlb_release_device_memory(struct dlb_dev *dev)
{
	int i;

	for (i = 0; i < DLB_MAX_NUM_DOMAINS; i++)
		dlb_release_domain_memory(dev, i);
}
