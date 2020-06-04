// SPDX-License-Identifier: GPL-2.0-only
/*
 * Copyright © 2015 Intel Corporation.
 *
 * Authors: David Woodhouse <dwmw2@infradead.org>
 */

#include <linux/intel-iommu.h>
#include <linux/mmu_notifier.h>
#include <linux/sched.h>
#include <linux/sched/mm.h>
#include <linux/slab.h>
#include <linux/intel-svm.h>
#include <linux/rculist.h>
#include <linux/pci.h>
#include <linux/pci-ats.h>
#include <linux/dmar.h>
#include <linux/interrupt.h>
#include <linux/mm_types.h>
#include <linux/ioasid.h>
#include <asm/page.h>

#include "intel-pasid.h"

static int intel_svm_process_prq(struct intel_iommu *iommu,
				struct page_req_dsc *prq, int head, int tail);
static irqreturn_t prq_event_thread(int irq, void *d);

#define PRQ_ORDER 0

int intel_svm_enable_prq(struct intel_iommu *iommu)
{
	struct page *pages;
	int irq, ret;

	pages = alloc_pages(GFP_KERNEL | __GFP_ZERO, PRQ_ORDER);
	if (!pages) {
		pr_warn("IOMMU: %s: Failed to allocate page request queue\n",
			iommu->name);
		return -ENOMEM;
	}
	iommu->prq = page_address(pages);

	irq = dmar_alloc_hwirq(DMAR_UNITS_SUPPORTED + iommu->seq_id, iommu->node, iommu);
	if (irq <= 0) {
		pr_err("IOMMU: %s: Failed to create IRQ vector for page request queue\n",
		       iommu->name);
		ret = -EINVAL;
	err:
		free_pages((unsigned long)iommu->prq, PRQ_ORDER);
		iommu->prq = NULL;
		return ret;
	}
	iommu->pr_irq = irq;

	snprintf(iommu->prq_name, sizeof(iommu->prq_name), "dmar%d-prq", iommu->seq_id);

	ret = request_threaded_irq(irq, NULL, prq_event_thread, IRQF_ONESHOT,
				   iommu->prq_name, iommu);
	if (ret) {
		pr_err("IOMMU: %s: Failed to request IRQ for page request queue\n",
		       iommu->name);
		dmar_free_hwirq(irq);
		iommu->pr_irq = 0;
		goto err;
	}
	dmar_writeq(iommu->reg + DMAR_PQH_REG, 0ULL);
	dmar_writeq(iommu->reg + DMAR_PQT_REG, 0ULL);
	dmar_writeq(iommu->reg + DMAR_PQA_REG, virt_to_phys(iommu->prq) | PRQ_ORDER);

	return 0;
}

int intel_svm_finish_prq(struct intel_iommu *iommu)
{
	dmar_writeq(iommu->reg + DMAR_PQH_REG, 0ULL);
	dmar_writeq(iommu->reg + DMAR_PQT_REG, 0ULL);
	dmar_writeq(iommu->reg + DMAR_PQA_REG, 0ULL);

	if (iommu->pr_irq) {
		free_irq(iommu->pr_irq, iommu);
		dmar_free_hwirq(iommu->pr_irq);
		iommu->pr_irq = 0;
	}

	free_pages((unsigned long)iommu->prq, PRQ_ORDER);
	iommu->prq = NULL;

	return 0;
}

static inline bool intel_svm_capable(struct intel_iommu *iommu)
{
	return iommu->flags & VTD_FLAG_SVM_CAPABLE;
}

void intel_svm_check(struct intel_iommu *iommu)
{
	if (!pasid_supported(iommu))
		return;

	if (cpu_feature_enabled(X86_FEATURE_GBPAGES) &&
	    !cap_fl1gp_support(iommu->cap)) {
		pr_err("%s SVM disabled, incompatible 1GB page capability\n",
		       iommu->name);
		return;
	}

	if (cpu_feature_enabled(X86_FEATURE_LA57) &&
	    !cap_5lp_support(iommu->cap)) {
		pr_err("%s SVM disabled, incompatible paging mode\n",
		       iommu->name);
		return;
	}

	iommu->flags |= VTD_FLAG_SVM_CAPABLE;
}

static void intel_flush_svm_range_dev (struct intel_svm *svm, struct intel_svm_dev *sdev,
				unsigned long address, unsigned long pages, int ih)
{
	struct qi_desc desc;

	if (pages == -1) {
		desc.qw0 = QI_EIOTLB_PASID(svm->pasid) |
			QI_EIOTLB_DID(sdev->did) |
			QI_EIOTLB_GRAN(QI_GRAN_NONG_PASID) |
			QI_EIOTLB_TYPE;
		desc.qw1 = 0;
	} else {
		int mask = ilog2(__roundup_pow_of_two(pages));

		desc.qw0 = QI_EIOTLB_PASID(svm->pasid) |
				QI_EIOTLB_DID(sdev->did) |
				QI_EIOTLB_GRAN(QI_GRAN_PSI_PASID) |
				QI_EIOTLB_TYPE;
		desc.qw1 = QI_EIOTLB_ADDR(address) |
				QI_EIOTLB_IH(ih) |
				QI_EIOTLB_AM(mask);
	}
	desc.qw2 = 0;
	desc.qw3 = 0;
	qi_submit_sync(&desc, svm->iommu);

	if (sdev->dev_iotlb) {
		desc.qw0 = QI_DEV_EIOTLB_PASID(svm->pasid) |
				QI_DEV_EIOTLB_SID(sdev->sid) |
				QI_DEV_EIOTLB_QDEP(sdev->qdep) |
				QI_DEIOTLB_TYPE;
		if (pages == -1) {
			desc.qw1 = QI_DEV_EIOTLB_ADDR(-1ULL >> 1) |
					QI_DEV_EIOTLB_SIZE;
		} else if (pages > 1) {
			/* The least significant zero bit indicates the size. So,
			 * for example, an "address" value of 0x12345f000 will
			 * flush from 0x123440000 to 0x12347ffff (256KiB). */
			unsigned long last = address + ((unsigned long)(pages - 1) << VTD_PAGE_SHIFT);
			unsigned long mask = __rounddown_pow_of_two(address ^ last);

			desc.qw1 = QI_DEV_EIOTLB_ADDR((address & ~mask) |
					(mask - 1)) | QI_DEV_EIOTLB_SIZE;
		} else {
			desc.qw1 = QI_DEV_EIOTLB_ADDR(address);
		}
		desc.qw2 = 0;
		desc.qw3 = 0;
		qi_submit_sync(&desc, svm->iommu);
	}
}

static void intel_flush_svm_range(struct intel_svm *svm, unsigned long address,
				unsigned long pages, int ih)
{
	struct intel_svm_dev *sdev;

	rcu_read_lock();
	list_for_each_entry_rcu(sdev, &svm->devs, list)
		intel_flush_svm_range_dev(svm, sdev, address, pages, ih);
	rcu_read_unlock();
}

/* Pages have been freed at this point */
static void intel_invalidate_range(struct mmu_notifier *mn,
				   struct mm_struct *mm,
				   unsigned long start, unsigned long end)
{
	struct intel_svm *svm = container_of(mn, struct intel_svm, notifier);

	intel_flush_svm_range(svm, start,
			      (end - start + PAGE_SIZE - 1) >> VTD_PAGE_SHIFT, 0);
}

static void intel_mm_release(struct mmu_notifier *mn, struct mm_struct *mm)
{
	struct intel_svm *svm = container_of(mn, struct intel_svm, notifier);
	struct intel_svm_dev *sdev;

	/* This might end up being called from exit_mmap(), *before* the page
	 * tables are cleared. And __mmu_notifier_release() will delete us from
	 * the list of notifiers so that our invalidate_range() callback doesn't
	 * get called when the page tables are cleared. So we need to protect
	 * against hardware accessing those page tables.
	 *
	 * We do it by clearing the entry in the PASID table and then flushing
	 * the IOTLB and the PASID table caches. This might upset hardware;
	 * perhaps we'll want to point the PASID to a dummy PGD (like the zero
	 * page) so that we end up taking a fault that the hardware really
	 * *has* to handle gracefully without affecting other processes.
	 */
	rcu_read_lock();
	list_for_each_entry_rcu(sdev, &svm->devs, list) {
		intel_pasid_tear_down_entry(svm->iommu, sdev->dev, svm->pasid);
		intel_flush_svm_range_dev(svm, sdev, 0, -1, 0);
	}
	rcu_read_unlock();

}

static const struct mmu_notifier_ops intel_mmuops = {
	.release = intel_mm_release,
	.invalidate_range = intel_invalidate_range,
};

static DEFINE_MUTEX(pasid_mutex);
static LIST_HEAD(global_svm_list);

#define for_each_svm_dev(sdev, svm, d)			\
	list_for_each_entry((sdev), &(svm)->devs, list)	\
		if ((d) != (sdev)->dev) {} else

/*
 * If this mm already has a PASID we can use it. Otherwise allocate a new one.
 * Let the caller know if we did an allocation via 'new_pasid'.
 */
static int alloc_pasid(struct intel_svm *svm, struct mm_struct *mm,
		       int pasid_max,  bool *new_pasid, int flags)
{
	int pasid;

	/*
	 * Reuse the PASID if the mm already has a PASID and not a private
	 * PASID is requested.
	 */
	if (mm && mm->context.pasid && !(flags & SVM_FLAG_PRIVATE_PASID)) {
		/*
		 * Once a PASID is allocated for this mm, the PASID
		 * stays with the mm until the mm is dropped. Reuse
		 * the PASID which has been already allocated for the
		 * mm instead of allocating a new one.
		 */
		ioasid_set_data(mm->context.pasid, svm);
		*new_pasid = false;

		return mm->context.pasid;
	}

	/*
	 * Allocate a new pasid. Do not use PASID 0, reserved for RID to
	 * PASID.
	 */
	pasid = ioasid_alloc(NULL, PASID_MIN, pasid_max - 1, svm);
	if (pasid == INVALID_IOASID)
		return -ENOSPC;

	*new_pasid = true;

	return pasid;
}

static int ioasid_status_change(struct notifier_block *nb,
				unsigned long code, void *data)
{
	ioasid_t ioasid = *(ioasid_t *)data;
	struct intel_svm_dev *sdev;
	struct intel_svm *svm;

	if (code == IOASID_FREE) {
		/*
		 * Unbind all devices associated with this PASID which is
		 * being freed by other users such as VFIO.
		 */
		svm = ioasid_find(NULL, ioasid, NULL);
		if (!svm || !svm->iommu)
			return NOTIFY_DONE;

		if (IS_ERR(svm))
			return NOTIFY_BAD;

		mutex_lock(&pasid_mutex);
		list_for_each_entry_rcu(sdev, &svm->devs, list) {
			/* Does not poison forward pointer */
			list_del_rcu(&sdev->list);
			intel_pasid_tear_down_entry(svm->iommu, sdev->dev,
						    svm->pasid);
			kfree_rcu(sdev, rcu);

			if (list_empty(&svm->devs)) {
				list_del(&svm->list);
				ioasid_set_data(ioasid, NULL);
				kfree(svm);
			}
			synchronize_rcu();
		}
		mutex_unlock(&pasid_mutex);

		return NOTIFY_OK;
	}

	return NOTIFY_DONE;
}

static struct notifier_block svm_ioasid_notifier = {
		.notifier_call = ioasid_status_change,
};

#define PRQ_RING_MASK ((0x1000 << PRQ_ORDER) - 0x20)

/*
 * When a PASID is stopped or terminated, there can be pending PRQs
 * (requests have not received responses) in software, remapping
 * hardware, and devices.
 * SW tracked pending PRQs should have been cleared by common code
 * prior to this call. Here we drain the PRQs in hardware.
 *
 * There are two scenarios that PRQ drains are required:
 * 1. unbind a PASID
 * 2. resume phase of the PASID suspend/resume cycle.
 *
 * Here are the steps to be performed:
 * 0. Disable PR interrupt
 * 1. Take a snapshot of the page request queue, record current head and tail,
 *    and mark PRQ entries with PASID to be dropped
 * 2. Mark queue empty, Head = Tail.
 * 2. Perform drain as in VT-d spec. 7.11. No queue full check since queue was
 *    empty at the point of the drain.
 * 3. Tail could have moved due to new PRQ written by HW
 * 4. Process snapshot copy of PR queue
 * 5. Process hardware PR queue, enable interrupt
 * For an example, consider the following timeline going downward.
 *    VT-d HW            VT-d Driver          User(KMD, guest)
 * --------------------------------------------------------
 *   [PR1.2.3]
 *   [PR1.1.3] <tail>
 *   [PR1.2.2]
 *   [PR1.2.1]
 *   [PR1.1.2]
 *   [PR1.1.1] <head>
 *   [IRQ] ->
 *
 *
 *                                          <-  [unbind PASID 1]
 *                       [delete pending PR]
 *                       [drain PRQs]
 *
 *
 * Decoder:
 * PR.PASID.GroupID.Index,
 * e.g. PR.1.2.3 indicates the Page request with PASID = 1, GroupID = 2,
 * 3rd request in the group.
 * LPIG: last page in group
 * PDP: private data present
 * KMD: kernel mode driver for native SVA
 * Caller of unbind/suspend/resume PASID APIs must ensure no pending DMA
 * activities prior to call.
 */
static void intel_svm_drain_prq(struct device *dev, int pasid)
{
	struct intel_iommu *iommu = intel_svm_device_to_iommu(dev);
	struct device_domain_info *info;
	struct dmar_domain *domain;
	unsigned long flags;
	int head, tail;
	void *prqq;
	struct qi_desc desc[3];
	struct pci_dev *pdev;
	int qdep, rc = 0;
	u16 sid, did;
	struct intel_svm *svm = NULL;

	dev_dbg(dev, "%s: pasid %d\n", __func__, pasid);

	rcu_read_lock();
	svm = ioasid_find(NULL, pasid, NULL);
	if (!svm) {
		dev_err(dev, "PASID not found\n");
		return;
	}
	rcu_read_unlock();

	spin_lock_irqsave(&iommu->lock, flags);

	tail = dmar_readq(iommu->reg + DMAR_PQT_REG) & PRQ_RING_MASK;
	head = dmar_readq(iommu->reg + DMAR_PQH_REG) & PRQ_RING_MASK;

	/*
	 * Make a copy of the PR queue then process it offline without
	 * blocking PRQ interrupts.
	 */
	prqq = kmemdup(iommu->prq, PAGE_SIZE ^ PRQ_ORDER, GFP_ATOMIC);
	if (!prqq) {
		spin_unlock_irqrestore(&iommu->lock, flags);
		return;
	}
	/*
	 * Make queue empty to allow further events and avoid the queue full
	 * condition while we drain the queue.
	 */
	dmar_writeq(iommu->reg + DMAR_PQH_REG, tail);
	spin_unlock_irqrestore(&iommu->lock, flags);

	/* process the copy of PRQ, drained PASID already marked to be dropped */
	if (intel_svm_process_prq(iommu, prqq, head, tail))
		goto out_free;

	/*
	 * Perform steps prescribed in VT-d spec CH7.11 to drain page request
	 * and responses.
	 */
	/* step a, submit an IWD with fence bit */
	info = dev->archdata.iommu;
	sid = PCI_DEVID(info->bus, info->devfn);
	domain = info->domain;
	did = domain->iommu_did[iommu->seq_id];
	pdev = to_pci_dev(dev);
	qdep = pci_ats_queue_depth(pdev);

	/*
	 * TODO: As a potential optimization for vIOMMU, we only need to send
	 * IWD with drain D flag.
	 */
	memset(desc, 0, sizeof(desc));
	desc[0].qw0 = QI_IWD_STATUS_DATA(QI_DONE) | QI_IWD_FENCE |
		QI_IWD_TYPE;

	/* step b, submit IOTLB and DIOTLB invalidation, any granu */
	desc[1].qw0 = QI_EIOTLB_PASID(pasid) |
		QI_EIOTLB_DID(did) |
		QI_EIOTLB_GRAN(QI_GRAN_NONG_PASID)|
		QI_EIOTLB_TYPE;

	desc[2].qw0 = QI_DEV_EIOTLB_PASID(pasid) | QI_DEV_EIOTLB_SID(sid) |
		QI_DEV_EIOTLB_QDEP(qdep) | QI_DEIOTLB_TYPE |
		QI_DEV_IOTLB_PFSID(info->pfsid);

	rc = __qi_submit_sync(desc, iommu, 3, QI_OPT_WAIT_DRAIN);
	if (rc)
		dev_err(dev, "Failed to drain PRQ\n");

	/*
	 * If new requests come in while we processing the copy, we should
	 * process it now, otherwise the new request may be stuck until the
	 * next IRQ.
	 */
	if (dmar_readq(iommu->reg + DMAR_PRS_REG) & DMA_PRS_PPR) {
		tail = dmar_readq(iommu->reg + DMAR_PQT_REG) & PRQ_RING_MASK;
		head = dmar_readq(iommu->reg + DMAR_PQH_REG) & PRQ_RING_MASK;
		intel_svm_process_prq(iommu, iommu->prq, head, tail);
	}

	/* Allow new pending PRQ to generate interrupts */
	writel(DMA_PRS_PPR, iommu->reg + DMAR_PRS_REG);

out_free:
	kfree(prqq);

	return;
}

int intel_iommu_page_response(struct device *dev,
			struct iommu_fault_event *evt,
			struct iommu_page_response *msg)
{
	struct intel_iommu *iommu;
	struct intel_svm_dev *sdev;
	struct intel_svm *svm = NULL;
	struct iommu_fault_page_request *prm;
	u8 bus, devfn;
	u16 sid;
	unsigned long flags;
	bool sdev_found = false;
	struct qi_desc resp;
	int ret = 0;
printk("%s, - 1 \n", __func__);
	/*
	 * Yi: here use device_to_iommu() as may need to support
	 * page response without pasid present.
	 */
	iommu = device_to_iommu(dev, &bus, &devfn);
	if (WARN_ON(!iommu))
		return -ENODEV;

	if (!msg || !evt)
		return -EINVAL;

	/* TODO: may add version to struct page_response_msg */

	if (!dev || !dev_is_pci(dev))
		return -ENODEV;
	mutex_lock(&pasid_mutex);

printk("%s, - 2 \n", __func__);
	spin_lock_irqsave(&iommu->lock, flags);
	prm = &evt->fault.prm;
	sid = bus << 8 | devfn;
	if (!(prm->flags & IOMMU_FAULT_PAGE_REQUEST_PASID_VALID))
		goto submit_qi;

	/* VT-d supports devices with full 20 bit PASIDs only */
	if (pci_max_pasids(to_pci_dev(dev)) != PASID_MAX) {
		ret = -EINVAL;
		goto out;
	}

	if (prm->pasid <= 0 || prm->pasid >= PASID_MAX) {
		ret = -EINVAL;
		goto out;
	}

	/* REVISIT:
	 * Sanity check adddress width and paging mode support
	 * width matching in two dimensions:
	 * 1. paging mode CPU <= IOMMU
	 * 2. address width Guest <= Host.
	 */
	svm = ioasid_find(NULL, prm->pasid, NULL);
	if (IS_ERR(svm)) {
		ret = PTR_ERR(svm);
		goto out_unlock;
	}

	if (!svm)
		goto out_unlock;

	for_each_svm_dev(sdev, svm, dev) {
		/* Found a matched sdev, means this response is valide */
		sdev_found = true;
		break;
	}

	if (!sdev_found)
		goto out_unlock;
	/*
	 * Per VT-d spec. v3.0 ch7.7, system software must respond
	 * with page group response if private data is present (PDP)
	 * or last page in group (LPIG) bit is set. This is an
	 * additional VT-d feature beyond PCI ATS spec.
	 */
	/* TODO: we should never trust the infos from guest except for the resp code. This is tricky, but better to refer the infos from evt->fault. */
submit_qi:
	resp.qw0 = QI_PGRP_PASID(prm->pasid) |
		QI_PGRP_DID(sid) |
		QI_PGRP_PASID_P(prm->flags & IOMMU_FAULT_PAGE_REQUEST_PASID_VALID) |
		QI_PGRP_PDP(prm->flags & IOMMU_FAULT_PAGE_REQUEST_PRIV_DATA) |
		QI_PGRP_RESP_CODE(msg->code) |
		QI_PGRP_RESP_TYPE;
	resp.qw1 = QI_PGRP_IDX(prm->grpid) |
		QI_PGRP_LPIG(((prm->flags & IOMMU_FAULT_PAGE_REQUEST_LAST_PAGE) >> 1));
printk("%s, prm->flags: 0x%x\n", __func__, (unsigned) prm->flags);
	resp.qw2 = 0;
	resp.qw3 = 0;
	if (prm->flags & IOMMU_FAULT_PAGE_REQUEST_PRIV_DATA)
		memcpy(&resp.qw2, prm->private_data,
			sizeof(prm->private_data));

printk("%s, resp.qw0: 0x%lx, resp.qw1: 0x%lx, resp.qw2: 0x%lx, resp.qw3: 0x%lx\n", __func__, (unsigned long) resp.qw0, (unsigned long) resp.qw1, (unsigned long) resp.qw2, (unsigned long) resp.qw3);
	qi_submit_sync((struct qi_desc*)&resp, iommu);
out_unlock:
	mutex_unlock(&pasid_mutex);
out:
	spin_unlock_irqrestore(&iommu->lock, flags);
	return ret;
}

int intel_svm_bind_gpasid(struct iommu_domain *domain,
			struct device *dev,
			struct iommu_gpasid_bind_data *data)
{
	struct intel_iommu *iommu = intel_svm_device_to_iommu(dev);
	struct intel_svm_dev *sdev;
	struct intel_svm *svm = NULL;
	struct dmar_domain *ddomain;
	int ret = 0;

	if (WARN_ON(!iommu) || !data)
		return -EINVAL;

	if (data->version > IOMMU_UAPI_VERSION ||
	    data->format != IOMMU_PASID_FORMAT_INTEL_VTD)
		return -EINVAL;

	if (dev_is_pci(dev)) {
		/* VT-d supports devices with full 20 bit PASIDs only */
		if (pci_max_pasids(to_pci_dev(dev)) != PASID_MAX)
			return -EINVAL;
	} else {
		return -ENOTSUPP;
	}

	/*
	 * We only check host PASID range, we have no knowledge to check
	 * guest PASID range nor do we use the guest PASID.
	 */
	if (data->hpasid <= 0 || data->hpasid >= PASID_MAX)
		return -EINVAL;

	ddomain = to_dmar_domain(domain);

	/* Sanity check paging mode support match between host and guest */
	if (data->addr_width == ADDR_WIDTH_5LEVEL &&
	    !cap_5lp_support(iommu->cap)) {
		pr_err("Cannot support 5 level paging requested by guest!\n");
		return -EINVAL;
	}

	mutex_lock(&pasid_mutex);
	svm = ioasid_find(NULL, data->hpasid, NULL);
	if (IS_ERR(svm)) {
		ret = PTR_ERR(svm);
		goto out;
	}

	if (svm) {
		/*
		 * If we found svm for the PASID, there must be at
		 * least one device bond, otherwise svm should be freed.
		 */
		if (WARN_ON(list_empty(&svm->devs))) {
			ret = -EINVAL;
			goto out;
		}

		if (svm->mm == get_task_mm(current) &&
		    data->hpasid == svm->pasid &&
		    data->gpasid == svm->gpasid) {
			pr_warn("Cannot bind the same guest-host PASID for the same process\n");
			mmput(svm->mm);
			ret = -EINVAL;
			goto out;
		}
		mmput(current->mm);

		for_each_svm_dev(sdev, svm, dev) {
			/* In case of multiple sub-devices of the same pdev
			 * assigned, we should allow multiple bind calls with
			 * the same PASID and pdev.
			 */
			sdev->users++;
			goto out;
		}
	} else {
		/* We come here when PASID has never been bond to a device. */
		svm = kzalloc(sizeof(*svm), GFP_KERNEL);
		if (!svm) {
			ret = -ENOMEM;
			goto out;
		}
		/* REVISIT: upper layer/VFIO can track host process that bind the PASID.
		 * ioasid_set = mm might be sufficient for vfio to check pasid VMM
		 * ownership.
		 */
		svm->mm = get_task_mm(current);
		svm->pasid = data->hpasid;
		if (data->flags & IOMMU_SVA_GPASID_VAL) {
			svm->gpasid = data->gpasid;
			svm->flags |= SVM_FLAG_GUEST_PASID;
		}
		/* Get notified when IOASID is freed by others, e.g. VFIO */
		ret = ioasid_add_notifier(data->hpasid, &svm_ioasid_notifier);
		if (ret) {
			mmput(svm->mm);
			kfree(svm);
			goto out;
		}
		ioasid_set_data(data->hpasid, svm);
		INIT_LIST_HEAD_RCU(&svm->devs);
		mmput(svm->mm);
	}
	sdev = kzalloc(sizeof(*sdev), GFP_KERNEL);
	if (!sdev) {
		if (list_empty(&svm->devs)) {
			ioasid_set_data(data->hpasid, NULL);
			kfree(svm);
		}
		ret = -ENOMEM;
		goto out;
	}
	sdev->dev = dev;
	sdev->users = 1;

	/* Set up device context entry for PASID if not enabled already */
	ret = intel_iommu_enable_pasid(iommu, sdev->dev);
	if (ret) {
		dev_err(dev, "Failed to enable PASID capability\n");
		kfree(sdev);
		/*
		 * If this this a new PASID that never bond to a device, then
		 * the device list must be empty which indicates struct svm
		 * was allocated in this function.
		 */
		if (list_empty(&svm->devs)) {
			ioasid_set_data(data->hpasid, NULL);
			kfree(svm);
		}
		goto out;
	}

	/*
	 * For guest bind, we need to set up PASID table entry as follows:
	 * - FLPM matches guest paging mode
	 * - turn on nested mode
	 * - SL guest address width matching
	 */
	ret = intel_pasid_setup_nested(iommu,
				       dev,
				       (pgd_t *)data->gpgd,
				       data->hpasid,
				       &data->vtd,
				       ddomain,
				       data->addr_width);
	if (ret) {
		dev_err(dev, "Failed to set up PASID %llu in nested mode, Err %d\n",
			data->hpasid, ret);
		/*
		 * PASID entry should be in cleared state if nested mode
		 * set up failed. So we only need to clear IOASID tracking
		 * data such that free call will succeed.
		 */
		kfree(sdev);
		if (list_empty(&svm->devs)) {
			ioasid_set_data(data->hpasid, NULL);
			kfree(svm);
		}
		goto out;
	}
	svm->flags |= SVM_FLAG_GUEST_MODE;

	init_rcu_head(&sdev->rcu);
	list_add_rcu(&sdev->list, &svm->devs);
 out:
	mutex_unlock(&pasid_mutex);
	return ret;
}

int intel_svm_unbind_gpasid(struct device *dev, int pasid)
{
	struct intel_iommu *iommu = intel_svm_device_to_iommu(dev);
	struct intel_svm_dev *sdev;
	struct intel_svm *svm;
	int ret = -EINVAL;

	if (WARN_ON(!iommu))
		return -EINVAL;

	mutex_lock(&pasid_mutex);
	svm = ioasid_find(NULL, pasid, NULL);
	if (!svm) {
		ret = -EINVAL;
		goto out;
	}

	if (IS_ERR(svm)) {
		ret = PTR_ERR(svm);
		goto out;
	}

	for_each_svm_dev(sdev, svm, dev) {
		ret = 0;
		sdev->users--;
		if (!sdev->users) {
			list_del_rcu(&sdev->list);
			/* Drain in flight PRQ for the PASID since it
			 * may get reused soon, we don't want to
			 * confuse with its previous life.
			 * intel_svm_drain_prq(dev, pasid);
			 */
			intel_pasid_tear_down_entry(iommu, dev, svm->pasid);
/* REVISIT: Comment out since it doesn't work on QEMU, tested on Hypersim
			intel_svm_drain_prq(dev, pasid);
*/
			kfree_rcu(sdev, rcu);

			if (list_empty(&svm->devs)) {
				/*
				 * We do not free PASID here until explicit call
				 * from VFIO to free. The PASID life cycle
				 * management is largely tied to VFIO management
				 * of assigned device life cycles. In case of
				 * guest exit without a explicit free PASID call,
				 * the responsibility lies in VFIO layer to free
				 * the PASIDs allocated for the guest.
				 * For security reasons, VFIO has to track the
				 * PASID ownership per guest anyway to ensure
				 * that PASID allocated by one guest cannot be
				 * used by another.
				 */
				ioasid_remove_notifier(pasid,
						       &svm_ioasid_notifier);

				ioasid_set_data(pasid, NULL);
				kfree(svm);
			}
		}
		break;
	}
out:
	mutex_unlock(&pasid_mutex);

	return ret;
}

int intel_svm_bind_mm(struct device *dev, int *pasid, int flags, struct svm_dev_ops *ops)
{
	struct intel_iommu *iommu = intel_svm_device_to_iommu(dev);
	struct device_domain_info *info;
	struct intel_svm_dev *sdev;
	struct intel_svm *svm = NULL;
	struct mm_struct *mm = NULL;
	int pasid_max;
	int ret;

	if (!iommu || dmar_disabled)
		return -EINVAL;

	if (!intel_svm_capable(iommu))
		return -ENOTSUPP;

	if (dev_is_pci(dev)) {
		pasid_max = pci_max_pasids(to_pci_dev(dev));
		if (pasid_max < 0)
			return -EINVAL;
	} else
		pasid_max = 1 << 20;

	if (flags & SVM_FLAG_SUPERVISOR_MODE) {
		if (!ecap_srs(iommu->ecap))
			return -EINVAL;
	} else if (pasid) {
		mm = get_task_mm(current);
		BUG_ON(!mm);
	}

	mutex_lock(&pasid_mutex);
	if (pasid && !(flags & SVM_FLAG_PRIVATE_PASID)) {
		struct intel_svm *t;

		list_for_each_entry(t, &global_svm_list, list) {
			if (t->mm != mm || (t->flags & SVM_FLAG_PRIVATE_PASID))
				continue;

			svm = t;
			if (svm->pasid >= pasid_max) {
				dev_warn(dev,
					 "Limited PASID width. Cannot use existing PASID %d\n",
					 svm->pasid);
				ret = -ENOSPC;
				goto out;
			}

			/* Find the matching device in svm list */
			for_each_svm_dev(sdev, svm, dev) {
				if (sdev->ops != ops) {
					ret = -EBUSY;
					goto out;
				}
				sdev->users++;
				goto success;
			}

			break;
		}
	}

	sdev = kzalloc(sizeof(*sdev), GFP_KERNEL);
	if (!sdev) {
		ret = -ENOMEM;
		goto out;
	}
	sdev->dev = dev;

	ret = intel_iommu_enable_pasid(iommu, dev);
	if (ret || !pasid) {
		/* If they don't actually want to assign a PASID, this is
		 * just an enabling check/preparation. */
		kfree(sdev);
		goto out;
	}

	info = dev->archdata.iommu;
	if (!info || !info->pasid_supported) {
		kfree(sdev);
		goto out;
	}

	sdev->did = FLPT_DEFAULT_DID;
	sdev->sid = PCI_DEVID(info->bus, info->devfn);
	if (info->ats_enabled) {
		sdev->dev_iotlb = 1;
		sdev->qdep = info->ats_qdep;
		if (sdev->qdep >= QI_DEV_EIOTLB_MAX_INVS)
			sdev->qdep = 0;
	}

	/* Finish the setup now we know we're keeping it */
	sdev->users = 1;
	sdev->ops = ops;
	init_rcu_head(&sdev->rcu);

	if (!svm) {
		bool new_pasid;

		svm = kzalloc(sizeof(*svm), GFP_KERNEL);
		if (!svm) {
			ret = -ENOMEM;
			kfree(sdev);
			goto out;
		}
		svm->iommu = iommu;

		if (pasid_max > intel_pasid_max_id)
			pasid_max = intel_pasid_max_id;

		svm->pasid = alloc_pasid(svm, mm, pasid_max, &new_pasid, flags);
		if (svm->pasid < 0) {
			kfree(svm);
			kfree(sdev);
			goto out;
		}

		svm->notifier.ops = &intel_mmuops;
		svm->mm = mm;
		svm->flags = flags;
		INIT_LIST_HEAD_RCU(&svm->devs);
		INIT_LIST_HEAD(&svm->list);
		ret = -ENOMEM;
		if (mm) {
			ret = mmu_notifier_register(&svm->notifier, mm);
			if (ret) {
				if (new_pasid)
					ioasid_free(svm->pasid);
				kfree(svm);
				kfree(sdev);
				goto out;
			}
		}

		spin_lock(&iommu->lock);
		ret = intel_pasid_setup_first_level(iommu, dev,
				mm ? mm->pgd : init_mm.pgd,
				svm->pasid, FLPT_DEFAULT_DID,
				(mm ? 0 : PASID_FLAG_SUPERVISOR_MODE) |
				(cpu_feature_enabled(X86_FEATURE_LA57) ?
				 PASID_FLAG_FL5LP : 0));
		spin_unlock(&iommu->lock);
		if (ret) {
			if (mm)
				mmu_notifier_unregister(&svm->notifier, mm);
			if (new_pasid)
				ioasid_free(svm->pasid);
			kfree(svm);
			kfree(sdev);
			goto out;
		}

		if (mm && new_pasid && !(flags & SVM_FLAG_PRIVATE_PASID)) {
			/*
			 * Track the new pasid in the mm. The pasid will be
			 * freed at process exit. Don't track requested
			 * private PASID in the mm.
			 */
			mm->context.pasid = svm->pasid;
		}
		list_add_tail(&svm->list, &global_svm_list);
	} else {
		/*
		 * Binding a new device with existing PASID, need to setup
		 * the PASID entry.
		 */
		spin_lock(&iommu->lock);
		ret = intel_pasid_setup_first_level(iommu, dev,
						mm ? mm->pgd : init_mm.pgd,
						svm->pasid, FLPT_DEFAULT_DID,
						(mm ? 0 : PASID_FLAG_SUPERVISOR_MODE) |
						(cpu_feature_enabled(X86_FEATURE_LA57) ?
						PASID_FLAG_FL5LP : 0));
		spin_unlock(&iommu->lock);
		if (ret) {
			kfree(sdev);
			goto out;
		}
	}
	list_add_rcu(&sdev->list, &svm->devs);

 success:
	*pasid = svm->pasid;
	ret = 0;
 out:
	mutex_unlock(&pasid_mutex);
	if (mm)
		mmput(mm);
	return ret;
}
EXPORT_SYMBOL_GPL(intel_svm_bind_mm);

int intel_svm_unbind_mm(struct device *dev, int pasid)
{
	struct intel_svm_dev *sdev;
	struct intel_iommu *iommu;
	struct intel_svm *svm;
	int ret = -EINVAL;

	mutex_lock(&pasid_mutex);
	iommu = intel_svm_device_to_iommu(dev);
	if (!iommu)
		goto out;

	svm = ioasid_find(NULL, pasid, NULL);
	if (!svm)
		goto out;

	if (IS_ERR(svm)) {
		ret = PTR_ERR(svm);
		goto out;
	}

	for_each_svm_dev(sdev, svm, dev) {
		ret = 0;
		sdev->users--;
		if (!sdev->users) {
			list_del_rcu(&sdev->list);
			/* Flush the PASID cache and IOTLB for this device.
			 * Note that we do depend on the hardware *not* using
			 * the PASID any more. Just as we depend on other
			 * devices never using PASIDs that they have no right
			 * to use. We have a *shared* PASID table, because it's
			 * large and has to be physically contiguous. So it's
			 * hard to be as defensive as we might like. */
			intel_pasid_tear_down_entry(iommu, dev, svm->pasid);
			intel_flush_svm_range_dev(svm, sdev, 0, -1, 0);
			kfree_rcu(sdev, rcu);

			if (list_empty(&svm->devs)) {
				/* Clear data in the pasid. */
				ioasid_set_data(pasid, NULL);
				if (svm->mm)
					mmu_notifier_unregister(&svm->notifier, svm->mm);
				list_del(&svm->list);
				/* We mandate that no page faults may be outstanding
				 * for the PASID when intel_svm_unbind_mm() is called.
				 * If that is not obeyed, subtle errors will happen.
				 * Let's make them less subtle... */
				memset(svm, 0x6b, sizeof(*svm));
				kfree(svm);
			}
		}
		break;
	}
 out:
	mutex_unlock(&pasid_mutex);

	return ret;
}
EXPORT_SYMBOL_GPL(intel_svm_unbind_mm);

int intel_svm_is_pasid_valid(struct device *dev, int pasid)
{
	struct intel_iommu *iommu;
	struct intel_svm *svm;
	int ret = -EINVAL;

	mutex_lock(&pasid_mutex);
	iommu = intel_svm_device_to_iommu(dev);
	if (!iommu)
		goto out;

	svm = ioasid_find(NULL, pasid, NULL);
	if (!svm)
		goto out;

	if (IS_ERR(svm)) {
		ret = PTR_ERR(svm);
		goto out;
	}
	/* init_mm is used in this case */
	if (!svm->mm)
		ret = 1;
	else if (atomic_read(&svm->mm->mm_users) > 0)
		ret = 1;
	else
		ret = 0;

 out:
	mutex_unlock(&pasid_mutex);

	return ret;
}
EXPORT_SYMBOL_GPL(intel_svm_is_pasid_valid);

/* Page request queue descriptor */
struct page_req_dsc {
	union {
		struct {
			u64 type:8;
			u64 pasid_present:1;
			u64 priv_data_present:1;
			u64 rsvd:6;
			u64 rid:16;
			u64 pasid:20;
			u64 exe_req:1;
			u64 pm_req:1;
			u64 rsvd2:10;
		};
		u64 qw_0;
	};
	union {
		struct {
			u64 rd_req:1;
			u64 wr_req:1;
			u64 lpig:1;
			u64 prg_index:9;
			u64 addr:52;
		};
		u64 qw_1;
	};
	u64 priv_data[2];
};

#define PRQ_RING_MASK	((0x1000 << PRQ_ORDER) - 0x20)

static bool access_error(struct vm_area_struct *vma, struct page_req_dsc *req)
{
	unsigned long requested = 0;

	if (req->exe_req)
		requested |= VM_EXEC;

	if (req->rd_req)
		requested |= VM_READ;

	if (req->wr_req)
		requested |= VM_WRITE;

	return (requested & ~vma->vm_flags) != 0;
}

static bool is_canonical_address(u64 addr)
{
	int shift = 64 - (__VIRTUAL_MASK_SHIFT + 1);
	long saddr = (long) addr;

	return (((saddr << shift) >> shift) == saddr);
}

static int prq_to_iommu_prot(struct page_req_dsc *req)
{
	int prot = 0;

	if (req->rd_req)
		prot |= IOMMU_FAULT_READ;
	if (req->wr_req)
		prot |= IOMMU_FAULT_WRITE;
	if (req->exe_req)
		prot |= IOMMU_FAULT_EXEC;
	if (req->pm_req)
		prot |= IOMMU_FAULT_PRIV;

	return prot;
}

/* REVISIT: use common code if any */
#define RID_TO_BUS(x) (x) >> 8
#define RID_TO_DEVFN(x) (x) & 0xFF

/* REVISIT:
 * SW does explicit TLB flush for each page request completed.
 * For guest vIOMMU, only send page response back, no TLB flush
 * send to the host since host will receive page response.
 * Consider the following timeline going downward.
 *    Host              vIOMMU                Guest
 * --------------------------------------------------------
 *   [PRQ0]->
 *   <track>
 *                      inject->
 *                                            [PRQ0]
 *                                            handle_mm_fault
 *                                            [IOTLB flush]*
 *   [PRQ1]->
 *   <track>
 *                      inject->
 *                                            [PRQ1]
 *                                            handle_mm_fault
 *                                            [IOTLB flush]*
 *   [PRQ2 PDP2]->
 *   <track>
 *                      inject->
 *   [PRQ3 PDP3]->
 *   <track>
 *                      inject->
 *                                            [PRQ2]
 *                                            handle_mm_fault
 *                                          <-[IOTLB flush]*
 *                                          <-[PG RSP PDP2]
 *                      <-passdown
 *   < if PDP2 matches >
 *   [PRQ0 IOTLB flush]
 *   [PRQ1 IOTLB flush]
 *   [PRQ2 IOTLB flush]
 *   [PRQ2 PG RSP PDP2]
 *                                            [PRQ3]
 *                                            handle_mm_fault
 *                                          <-[IOTLB flush]*
 *                                          <-[PG RSP PDP3]
 *                      <-passdown
 *   < if PDP3 matches >
 *   [PRQ3 IOTLB flush]
 *   [PRQ3 PG RSP PDP3]
 *   [PRQ4 LPIG]
 *   <track>
 *                      inject->
 *                                            [PRQ4]
 *                                            handle_mm_fault
 *                                          <-[IOTLB flush]*
 *                                          <-[PG RSP4]
 *                      <-passdown
 *   < if LPIG >
 *   [PRQ4 IOTLB flush]
 *   [PRQ4 PG RSP]
 *
 *  * For optimization in guest, we skip IOTLB flush
 */
static int intel_svm_prq_report(struct intel_iommu *iommu,
				struct page_req_dsc *desc)
{
	int ret = 0;
	u8 bus, devfn;
	struct device_domain_info *info;
	struct iommu_fault_event event;
	struct pci_dev *pdev;

	memset(&event, 0, sizeof(struct iommu_fault_event));
	bus = RID_TO_BUS(desc->rid);
	devfn = RID_TO_DEVFN(desc->rid);
	pdev = pci_get_domain_bus_and_slot(iommu->segment, bus, devfn);

	if (!pdev) {
		pr_err("No PCI device found for PRQ [%02x:%02x.%d]\n",
			bus, PCI_SLOT(devfn), PCI_FUNC(devfn));
		return -ENODEV;
	}
	dev_dbg(&pdev->dev, "%s:\n", __func__);

	/* Fill in event data for device specific processing */
	event.fault.type = IOMMU_FAULT_PAGE_REQ;
	event.fault.prm.addr = (u64)desc->addr << VTD_PAGE_SHIFT;
	event.fault.prm.pasid = desc->pasid;
	event.fault.prm.grpid = desc->prg_index;
	event.fault.prm.perm = prq_to_iommu_prot(desc);

	/*
	 * Set last page in group bit if private data is present,
	 * page response is required as it does for LPIG.
	 */
	if (desc->lpig)
		event.fault.prm.flags |= IOMMU_FAULT_PAGE_REQUEST_LAST_PAGE;
	if (desc->priv_data_present)
		event.fault.prm.flags |= IOMMU_FAULT_PAGE_REQUEST_LAST_PAGE;

	event.fault.prm.flags |= IOMMU_FAULT_PAGE_REQUEST_PASID_VALID;
	/*
	 * keep track of PRQs with private data such that when the
	 * response comes back from the guest, we can match them
	 * and send page response accordingly. Within the page
	 * request group, private data can be different or the same.
	 * It is device's responsibility to come up with a scheme
	 * that works for its internal contexts and PRQ groups.
	 */
	if (desc->priv_data_present)
		memcpy(event.fault.prm.private_data, desc->priv_data,
			sizeof(desc->priv_data));
	printk("%s, prm flags: 0x%x grpid: 0x%x addr %llx\n", __func__,
		(unsigned) event.fault.prm.flags,
		(unsigned) event.fault.prm.grpid,
		event.fault.prm.addr);
	/*
	 * If the device supports PASID granu scalable mode, reports the
	 * PASID as vector such that handlers can be dispatched with per
	 * vector data.
	 */
	info = pdev->dev.archdata.iommu;
	if (!list_empty(&info->auxiliary_domains)) {
		dev_dbg(&pdev->dev, "Aux domain present, assign vector %d\n", desc->pasid);
		event.vector = desc->pasid;
	}
	ret = iommu_report_device_fault(&pdev->dev, &event);
	pci_dev_put(pdev);

	return ret;
}

static int intel_svm_process_prq(struct intel_iommu *iommu,
				struct page_req_dsc *prq, int head, int tail)
{
	struct intel_svm *svm = NULL;
	struct vm_area_struct *vma;
	struct intel_svm_dev *sdev;
	struct page_req_dsc *req;
	struct qi_desc resp;
	int ret = 0, result;
	u64 address;

	pr_debug("%s: desc: %llx, head: %x, tail: %x\n", __func__, (u64)prq, head, tail);
	while (head != tail) {
		u8 bus, devfn;

		req = &prq[head / sizeof(*req)];
		pr_debug("%s: req: %llx, head: %x, tail: %x\n", __func__, (u64)req, head, tail);
		/*
		 * Use invalid response instead of failure since this is not
		 * catastrophic case. We will not disable PRI for the device.
		 * PCIe ATS spec. 4.2.1.
		 */
		result = QI_RESP_INVALID;
		address = (u64)req->addr << VTD_PAGE_SHIFT;
		bus = RID_TO_BUS(req->rid);
		devfn = RID_TO_DEVFN(req->rid);
		if (!req->pasid_present) {
			pr_err("%s: Page request without PASID: %08llx %08llx\n",
			       iommu->name, ((unsigned long long *)req)[0],
			       ((unsigned long long *)req)[1]);
			goto no_pasid;
		}

		if (!svm || svm->pasid != req->pasid) {
			rcu_read_lock();
			svm = ioasid_find(NULL, req->pasid, NULL);
			/* It *can't* go away, because the driver is not permitted
			 * to unbind the mm while any page faults are outstanding.
			 * So we only need RCU to protect the internal idr code. */
			rcu_read_unlock();
			if (IS_ERR_OR_NULL(svm)) {
				pr_err("%s: Page request for invalid PASID %d: %08llx %08llx\n",
				       iommu->name, req->pasid, ((unsigned long long *)req)[0],
				       ((unsigned long long *)req)[1]);
				goto no_pasid;
			}
		}
		/* If address is not canonical, return invalid response */
		if (!is_canonical_address(address))
			goto bad_req;
		/* Ignore suspended PASIDs, drop them */
/* Yi: comment out for debug
		if (svm->flags & SVM_FLAG_SUSPENDED)
			goto prq_advance;
*/
		/*
		 * If prq is to be handled outside iommu driver via receiver of
		 * the fault notifiers, we skip the page response here.
		 */
		if (svm->flags & SVM_FLAG_GUEST_MODE && !intel_svm_prq_report(iommu, req)) {
			pr_debug("%s: report PRQ out\n", __func__);
			goto prq_advance;
		}
		result = QI_RESP_INVALID;
		/* Since we're using init_mm.pgd directly, we should never take
		 * any faults on kernel addresses. */
		if (!svm->mm)
			goto bad_req;

		/* If the mm is already defunct, don't handle faults. */
		if (!mmget_not_zero(svm->mm))
			goto bad_req;

		down_read(&svm->mm->mmap_sem);
		vma = find_extend_vma(svm->mm, address);
		if (!vma || address < vma->vm_start)
			goto invalid;

		if (access_error(vma, req))
			goto invalid;

		ret = handle_mm_fault(vma, address,
				      req->wr_req ? FAULT_FLAG_WRITE : 0);
		if (ret & VM_FAULT_ERROR)
			goto invalid;

		result = QI_RESP_SUCCESS;
	invalid:
		up_read(&svm->mm->mmap_sem);
		mmput(svm->mm);
	bad_req:
		/* Accounting for major/minor faults? */
		rcu_read_lock();
		list_for_each_entry_rcu(sdev, &svm->devs, list) {
			/* REVISIT: or use req->rid */
			if (sdev->sid == PCI_DEVID(bus, devfn))
				break;
		}
		/* Other devices can go away, but the drivers are not permitted
		 * to unbind while any page faults might be in flight. So it's
		 * OK to drop the 'lock' here now we have it. */
		rcu_read_unlock();

		if (WARN_ON(&sdev->list == &svm->devs))
			sdev = NULL;
		/* We get here in the error case where the PASID lookup failed,
		   and these can be NULL. Do not use them below this point! */
		sdev = NULL;
		svm = NULL;
	no_pasid:
		if (req->lpig || req->priv_data_present) {
			/*
			 * Per VT-d spec. v3.0 ch7.7, system software must respond
			 * with page group response if private data is present (PDP)
			 * or last page in group (LPIG) bit is set. This is an
			 * additional VT-d feature beyond PCI ATS spec.
			 */
			resp.qw0 = QI_PGRP_PASID(req->pasid) |
				QI_PGRP_DID(req->rid) |
				QI_PGRP_PASID_P(req->pasid_present) |
				QI_PGRP_PDP(req->priv_data_present) |
				QI_PGRP_RESP_CODE(result) |
				QI_PGRP_RESP_TYPE;
			resp.qw1 = QI_PGRP_IDX(req->prg_index) |
				QI_PGRP_LPIG(req->lpig);

			if (req->priv_data_present)
				memcpy(&resp.qw2, req->priv_data,
				       sizeof(req->priv_data));
			resp.qw2 = 0;
			resp.qw3 = 0;
			qi_submit_sync((struct qi_desc*)&resp, iommu);
		}
	prq_advance:
		head = (head + sizeof(*req)) & PRQ_RING_MASK;
	}

	return ret;
}

static irqreturn_t prq_event_thread(int irq, void *d)
{
	struct intel_iommu *iommu = d;
	int head, tail, handled = 0;

	/* Clear PPR bit before reading head/tail registers, to
	 * ensure that we get a new interrupt if needed. */
	writel(DMA_PRS_PPR, iommu->reg + DMAR_PRS_REG);

	tail = dmar_readq(iommu->reg + DMAR_PQT_REG) & PRQ_RING_MASK;
	head = dmar_readq(iommu->reg + DMAR_PQH_REG) & PRQ_RING_MASK;

	if (!intel_svm_process_prq(iommu, iommu->prq, head, tail))
		handled = 1;
	/* make queue empty, head = tail */
	dmar_writeq(iommu->reg + DMAR_PQH_REG, tail);

	return IRQ_RETVAL(handled);
}

/* On process exit free the PASID (if one was allocated). */
void __free_pasid(struct mm_struct *mm)
{
	int pasid = mm->context.pasid;

	if (!pasid)
		return;

	/*
	 * Since the pasid is not bound to any svm by now, there is no race
	 * here with binding/unbinding and no need to protect the free
	 * operation by pasid_mutex.
	 */
	ioasid_free(pasid);
}

/*
 * Fix up the PASID MSR if possible.
 *
 * But if the #GP was due to another reason, a second #GP might be triggered
 * to force proper behavior.
 */
bool __fixup_pasid_exception(void)
{
	struct mm_struct *mm;
	bool ret = true;
	u64 pasid_msr;
	int pasid;

	mm = get_task_mm(current);
	/* This #GP was triggered from user mode. So mm cannot be NULL. */
	pasid = mm->context.pasid;
	/* Ensure this process has been bound to a PASID. */
	if (!pasid) {
		ret = false;
		goto out;
	}

	/* Check to see if the PASID MSR has already been set for this task. */
	rdmsrl(MSR_IA32_PASID, pasid_msr);
	if (pasid_msr & MSR_IA32_PASID_VALID) {
		ret = false;
		goto out;
	}

	/* Fix up the MSR. */
	wrmsrl(MSR_IA32_PASID, pasid | MSR_IA32_PASID_VALID);
out:
	mmput(mm);

	return ret;
}
