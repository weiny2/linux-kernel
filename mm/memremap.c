/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright(c) 2015 Intel Corporation. All rights reserved. */
#include <linux/device.h>
#include <linux/io.h>
#include <linux/kasan.h>
#include <linux/memory_hotplug.h>
#include <linux/mm.h>
#include <linux/pfn_t.h>
#include <linux/pkeys.h>
#include <linux/swap.h>
#include <linux/mmzone.h>
#include <linux/swapops.h>
#include <linux/types.h>
#include <linux/wait_bit.h>
#include <linux/xarray.h>
#include <uapi/asm-generic/mman-common.h>

#define PKEY_INVALID (INT_MIN)

static DEFINE_XARRAY(pgmap_array);

/*
 * The memremap() and memremap_pages() interfaces are alternately used
 * to map persistent memory namespaces. These interfaces place different
 * constraints on the alignment and size of the mapping (namespace).
 * memremap() can map individual PAGE_SIZE pages. memremap_pages() can
 * only map subsections (2MB), and at least one architecture (PowerPC)
 * the minimum mapping granularity of memremap_pages() is 16MB.
 *
 * The role of memremap_compat_align() is to communicate the minimum
 * arch supported alignment of a namespace such that it can freely
 * switch modes without violating the arch constraint. Namely, do not
 * allow a namespace to be PAGE_SIZE aligned since that namespace may be
 * reconfigured into a mode that requires SUBSECTION_SIZE alignment.
 */
#ifndef CONFIG_ARCH_HAS_MEMREMAP_COMPAT_ALIGN
unsigned long memremap_compat_align(void)
{
	return SUBSECTION_SIZE;
}
EXPORT_SYMBOL_GPL(memremap_compat_align);
#endif

#ifdef CONFIG_DEV_PAGEMAP_OPS
DEFINE_STATIC_KEY_FALSE(devmap_managed_key);
EXPORT_SYMBOL(devmap_managed_key);
static atomic_t devmap_managed_enable;

static void devmap_managed_enable_put(void)
{
	if (atomic_dec_and_test(&devmap_managed_enable))
		static_branch_disable(&devmap_managed_key);
}

static int devmap_managed_enable_get(struct dev_pagemap *pgmap)
{
	if (pgmap->type == MEMORY_DEVICE_PRIVATE &&
	    (!pgmap->ops || !pgmap->ops->page_free)) {
		WARN(1, "Missing page_free method\n");
		return -EINVAL;
	}

	if (atomic_inc_return(&devmap_managed_enable) == 1)
		static_branch_enable(&devmap_managed_key);
	return 0;
}
#else
static int devmap_managed_enable_get(struct dev_pagemap *pgmap)
{
	return -EINVAL;
}
static void devmap_managed_enable_put(void)
{
}
#endif /* CONFIG_DEV_PAGEMAP_OPS */

#ifdef CONFIG_ZONE_DEVICE_ACCESS_PROTECTION
/*
 * Note all devices which have asked for protections share the same key.  The
 * key may, or may not, have been provided by the core.  If not, protection
 * will remain disabled.  The key acquisition is attempted at init time and
 * never again.  So we don't have to worry about dev_page_pkey changing.
 */
static int dev_page_pkey = PKEY_INVALID;
DEFINE_STATIC_KEY_FALSE(dev_protection_static_key);
EXPORT_SYMBOL(dev_protection_static_key);
DEFINE_MUTEX(dev_prot_enable_lock);
static int dev_protection_enable;

static pgprot_t dev_protection_enable_get(struct dev_pagemap *pgmap, pgprot_t prot)
{
	if (pgmap->flags & PGMAP_PROT_ENABLED && dev_page_pkey != PKEY_INVALID) {
		pgprotval_t val = pgprot_val(prot);

		mutex_lock(&dev_prot_enable_lock);
		dev_protection_enable++;
		/* Only enable the static branch 1 time */
		if (dev_protection_enable == 1)
			static_branch_enable(&dev_protection_static_key);
		mutex_unlock(&dev_prot_enable_lock);

		prot = __pgprot(val | _PAGE_PKEY(dev_page_pkey));
	}
	return prot;
}

static void dev_protection_enable_put(struct dev_pagemap *pgmap)
{
	if (pgmap->flags & PGMAP_PROT_ENABLED && dev_page_pkey != PKEY_INVALID) {
		mutex_lock(&dev_prot_enable_lock);
		dev_protection_enable--;
		if (dev_protection_enable == 0)
			static_branch_disable(&dev_protection_static_key);
		mutex_unlock(&dev_prot_enable_lock);
	}
}

void dev_access_disable(void)
{
	unsigned long flags;

	if (!static_branch_unlikely(&dev_protection_static_key))
		return;

	local_irq_save(flags);
	current->dev_page_access_ref--;
	if (current->dev_page_access_ref == 0)
		pks_update_protection(dev_page_pkey, PKEY_DISABLE_ACCESS);
	local_irq_restore(flags);
}
EXPORT_SYMBOL_GPL(dev_access_disable);

void dev_access_enable(void)
{
	unsigned long flags;

	if (!static_branch_unlikely(&dev_protection_static_key))
		return;

	local_irq_save(flags);
	/* 0 clears the PKEY_DISABLE_ACCESS bit, allowing access */
	if (current->dev_page_access_ref == 0)
		pks_update_protection(dev_page_pkey, 0);
	current->dev_page_access_ref++;
	local_irq_restore(flags);
}
EXPORT_SYMBOL_GPL(dev_access_enable);

/**
 * dev_access_protection_init: Configure a PKS key domain for device pages
 *
 * The domain defaults to the protected state.  Device page mappings should set
 * the PGMAP_PROT_ENABLED flag when mapping pages.
 *
 * Note the pkey is never free'ed.  This is run at init time and we either get
 * the key or we do not.  We need to do this to maintian a constant key (or
 * not) as device memory is added or removed.
 */
static int __init __dev_access_protection_init(void)
{
	int pkey = pks_key_alloc("Device Memory");

	if (pkey < 0)
		return 0;

	dev_page_pkey = pkey;

	return 0;
}
subsys_initcall(__dev_access_protection_init);
#else
static pgprot_t dev_protection_enable_get(struct dev_pagemap *pgmap, pgprot_t prot)
{
	return prot;
}
static void dev_protection_enable_put(struct dev_pagemap *pgmap)
{
}
#endif /* CONFIG_ZONE_DEVICE_ACCESS_PROTECTION */

static void pgmap_array_delete(struct resource *res)
{
	xa_store_range(&pgmap_array, PHYS_PFN(res->start), PHYS_PFN(res->end),
			NULL, GFP_KERNEL);
	synchronize_rcu();
}

static unsigned long pfn_first(struct dev_pagemap *pgmap)
{
	return PHYS_PFN(pgmap->res.start) +
		vmem_altmap_offset(pgmap_altmap(pgmap));
}

static unsigned long pfn_end(struct dev_pagemap *pgmap)
{
	const struct resource *res = &pgmap->res;

	return (res->start + resource_size(res)) >> PAGE_SHIFT;
}

static unsigned long pfn_next(unsigned long pfn)
{
	if (pfn % 1024 == 0)
		cond_resched();
	return pfn + 1;
}

#define for_each_device_pfn(pfn, map) \
	for (pfn = pfn_first(map); pfn < pfn_end(map); pfn = pfn_next(pfn))

static void dev_pagemap_kill(struct dev_pagemap *pgmap)
{
	if (pgmap->ops && pgmap->ops->kill)
		pgmap->ops->kill(pgmap);
	else
		percpu_ref_kill(pgmap->ref);
}

static void dev_pagemap_cleanup(struct dev_pagemap *pgmap)
{
	if (pgmap->ops && pgmap->ops->cleanup) {
		pgmap->ops->cleanup(pgmap);
	} else {
		wait_for_completion(&pgmap->done);
		percpu_ref_exit(pgmap->ref);
	}
	/*
	 * Undo the pgmap ref assignment for the internal case as the
	 * caller may re-enable the same pgmap.
	 */
	if (pgmap->ref == &pgmap->internal_ref)
		pgmap->ref = NULL;
}

void memunmap_pages(struct dev_pagemap *pgmap)
{
	struct resource *res = &pgmap->res;
	struct page *first_page;
	unsigned long pfn;
	int nid;

	dev_pagemap_kill(pgmap);
	for_each_device_pfn(pfn, pgmap)
		put_page(pfn_to_page(pfn));
	dev_pagemap_cleanup(pgmap);

	/* make sure to access a memmap that was actually initialized */
	first_page = pfn_to_page(pfn_first(pgmap));

	/* pages are dead and unused, undo the arch mapping */
	nid = page_to_nid(first_page);

	mem_hotplug_begin();
	remove_pfn_range_from_zone(page_zone(first_page), PHYS_PFN(res->start),
				   PHYS_PFN(resource_size(res)));
	if (pgmap->type == MEMORY_DEVICE_PRIVATE) {
		__remove_pages(PHYS_PFN(res->start),
			       PHYS_PFN(resource_size(res)), NULL);
	} else {
		arch_remove_memory(nid, res->start, resource_size(res),
				pgmap_altmap(pgmap));
		kasan_remove_zero_shadow(__va(res->start), resource_size(res));
	}
	mem_hotplug_done();

	untrack_pfn(NULL, PHYS_PFN(res->start), resource_size(res));
	pgmap_array_delete(res);
	WARN_ONCE(pgmap->altmap.alloc, "failed to free all reserved pages\n");
	devmap_managed_enable_put();
	dev_protection_enable_put(pgmap);
}
EXPORT_SYMBOL_GPL(memunmap_pages);

static void devm_memremap_pages_release(void *data)
{
	memunmap_pages(data);
}

static void dev_pagemap_percpu_release(struct percpu_ref *ref)
{
	struct dev_pagemap *pgmap =
		container_of(ref, struct dev_pagemap, internal_ref);

	complete(&pgmap->done);
}

/*
 * Not device managed version of dev_memremap_pages, undone by
 * memunmap_pages().  Please use dev_memremap_pages if you have a struct
 * device available.
 */
void *memremap_pages(struct dev_pagemap *pgmap, int nid)
{
	struct resource *res = &pgmap->res;
	struct dev_pagemap *conflict_pgmap;
	struct mhp_params params = {
		/*
		 * We do not want any optional features only our own memmap
		 */
		.altmap = pgmap_altmap(pgmap),
		.pgprot = PAGE_KERNEL,
	};
	int error, is_ram;
	bool need_devmap_managed = true;

	params.pgprot = dev_protection_enable_get(pgmap, params.pgprot);

	switch (pgmap->type) {
	case MEMORY_DEVICE_PRIVATE:
		if (!IS_ENABLED(CONFIG_DEVICE_PRIVATE)) {
			WARN(1, "Device private memory not supported\n");
			return ERR_PTR(-EINVAL);
		}
		if (!pgmap->ops || !pgmap->ops->migrate_to_ram) {
			WARN(1, "Missing migrate_to_ram method\n");
			return ERR_PTR(-EINVAL);
		}
		if (!pgmap->owner) {
			WARN(1, "Missing owner\n");
			return ERR_PTR(-EINVAL);
		}
		break;
	case MEMORY_DEVICE_FS_DAX:
		if (!IS_ENABLED(CONFIG_ZONE_DEVICE) ||
		    IS_ENABLED(CONFIG_FS_DAX_LIMITED)) {
			WARN(1, "File system DAX not supported\n");
			return ERR_PTR(-EINVAL);
		}
		break;
	case MEMORY_DEVICE_DEVDAX:
		need_devmap_managed = false;
		break;
	case MEMORY_DEVICE_PCI_P2PDMA:
		params.pgprot = pgprot_noncached(params.pgprot);
		need_devmap_managed = false;
		break;
	default:
		WARN(1, "Invalid pgmap type %d\n", pgmap->type);
		break;
	}

	if (!pgmap->ref) {
		if (pgmap->ops && (pgmap->ops->kill || pgmap->ops->cleanup))
			return ERR_PTR(-EINVAL);

		init_completion(&pgmap->done);
		error = percpu_ref_init(&pgmap->internal_ref,
				dev_pagemap_percpu_release, 0, GFP_KERNEL);
		if (error)
			return ERR_PTR(error);
		pgmap->ref = &pgmap->internal_ref;
	} else {
		if (!pgmap->ops || !pgmap->ops->kill || !pgmap->ops->cleanup) {
			WARN(1, "Missing reference count teardown definition\n");
			return ERR_PTR(-EINVAL);
		}
	}

	if (need_devmap_managed) {
		error = devmap_managed_enable_get(pgmap);
		if (error)
			return ERR_PTR(error);
	}

	conflict_pgmap = get_dev_pagemap(PHYS_PFN(res->start), NULL);
	if (conflict_pgmap) {
		WARN(1, "Conflicting mapping in same section\n");
		put_dev_pagemap(conflict_pgmap);
		error = -ENOMEM;
		goto err_array;
	}

	conflict_pgmap = get_dev_pagemap(PHYS_PFN(res->end), NULL);
	if (conflict_pgmap) {
		WARN(1, "Conflicting mapping in same section\n");
		put_dev_pagemap(conflict_pgmap);
		error = -ENOMEM;
		goto err_array;
	}

	is_ram = region_intersects(res->start, resource_size(res),
		IORESOURCE_SYSTEM_RAM, IORES_DESC_NONE);

	if (is_ram != REGION_DISJOINT) {
		WARN_ONCE(1, "%s attempted on %s region %pr\n", __func__,
				is_ram == REGION_MIXED ? "mixed" : "ram", res);
		error = -ENXIO;
		goto err_array;
	}

	error = xa_err(xa_store_range(&pgmap_array, PHYS_PFN(res->start),
				PHYS_PFN(res->end), pgmap, GFP_KERNEL));
	if (error)
		goto err_array;

	if (nid < 0)
		nid = numa_mem_id();

	error = track_pfn_remap(NULL, &params.pgprot, PHYS_PFN(res->start),
				0, resource_size(res));
	if (error)
		goto err_pfn_remap;

	mem_hotplug_begin();

	/*
	 * For device private memory we call add_pages() as we only need to
	 * allocate and initialize struct page for the device memory. More-
	 * over the device memory is un-accessible thus we do not want to
	 * create a linear mapping for the memory like arch_add_memory()
	 * would do.
	 *
	 * For all other device memory types, which are accessible by
	 * the CPU, we do want the linear mapping and thus use
	 * arch_add_memory().
	 */
	if (pgmap->type == MEMORY_DEVICE_PRIVATE) {
		error = add_pages(nid, PHYS_PFN(res->start),
				PHYS_PFN(resource_size(res)), &params);
	} else {
		error = kasan_add_zero_shadow(__va(res->start), resource_size(res));
		if (error) {
			mem_hotplug_done();
			goto err_kasan;
		}

		error = arch_add_memory(nid, res->start, resource_size(res),
					&params);
	}

	if (!error) {
		struct zone *zone;

		zone = &NODE_DATA(nid)->node_zones[ZONE_DEVICE];
		move_pfn_range_to_zone(zone, PHYS_PFN(res->start),
				PHYS_PFN(resource_size(res)), params.altmap);
	}

	mem_hotplug_done();
	if (error)
		goto err_add_memory;

	/*
	 * Initialization of the pages has been deferred until now in order
	 * to allow us to do the work while not holding the hotplug lock.
	 */
	memmap_init_zone_device(&NODE_DATA(nid)->node_zones[ZONE_DEVICE],
				PHYS_PFN(res->start),
				PHYS_PFN(resource_size(res)), pgmap);
	percpu_ref_get_many(pgmap->ref, pfn_end(pgmap) - pfn_first(pgmap));
	return __va(res->start);

 err_add_memory:
	kasan_remove_zero_shadow(__va(res->start), resource_size(res));
 err_kasan:
	untrack_pfn(NULL, PHYS_PFN(res->start), resource_size(res));
 err_pfn_remap:
	pgmap_array_delete(res);
 err_array:
	dev_pagemap_kill(pgmap);
	dev_pagemap_cleanup(pgmap);
	devmap_managed_enable_put();
	return ERR_PTR(error);
}
EXPORT_SYMBOL_GPL(memremap_pages);

/**
 * devm_memremap_pages - remap and provide memmap backing for the given resource
 * @dev: hosting device for @res
 * @pgmap: pointer to a struct dev_pagemap
 *
 * Notes:
 * 1/ At a minimum the res and type members of @pgmap must be initialized
 *    by the caller before passing it to this function
 *
 * 2/ The altmap field may optionally be initialized, in which case
 *    PGMAP_ALTMAP_VALID must be set in pgmap->flags.
 *
 * 3/ The ref field may optionally be provided, in which pgmap->ref must be
 *    'live' on entry and will be killed and reaped at
 *    devm_memremap_pages_release() time, or if this routine fails.
 *
 * 4/ res is expected to be a host memory range that could feasibly be
 *    treated as a "System RAM" range, i.e. not a device mmio range, but
 *    this is not enforced.
 */
void *devm_memremap_pages(struct device *dev, struct dev_pagemap *pgmap)
{
	int error;
	void *ret;

	ret = memremap_pages(pgmap, dev_to_node(dev));
	if (IS_ERR(ret))
		return ret;

	error = devm_add_action_or_reset(dev, devm_memremap_pages_release,
			pgmap);
	if (error)
		return ERR_PTR(error);
	return ret;
}
EXPORT_SYMBOL_GPL(devm_memremap_pages);

void devm_memunmap_pages(struct device *dev, struct dev_pagemap *pgmap)
{
	devm_release_action(dev, devm_memremap_pages_release, pgmap);
}
EXPORT_SYMBOL_GPL(devm_memunmap_pages);

unsigned long vmem_altmap_offset(struct vmem_altmap *altmap)
{
	/* number of pfns from base where pfn_to_page() is valid */
	if (altmap)
		return altmap->reserve + altmap->free;
	return 0;
}

void vmem_altmap_free(struct vmem_altmap *altmap, unsigned long nr_pfns)
{
	altmap->alloc -= nr_pfns;
}

/**
 * get_dev_pagemap() - take a new live reference on the dev_pagemap for @pfn
 * @pfn: page frame number to lookup page_map
 * @pgmap: optional known pgmap that already has a reference
 *
 * If @pgmap is non-NULL and covers @pfn it will be returned as-is.  If @pgmap
 * is non-NULL but does not cover @pfn the reference to it will be released.
 */
struct dev_pagemap *get_dev_pagemap(unsigned long pfn,
		struct dev_pagemap *pgmap)
{
	resource_size_t phys = PFN_PHYS(pfn);

	/*
	 * In the cached case we're already holding a live reference.
	 */
	if (pgmap) {
		if (phys >= pgmap->res.start && phys <= pgmap->res.end)
			return pgmap;
		put_dev_pagemap(pgmap);
	}

	/* fall back to slow path lookup */
	rcu_read_lock();
	pgmap = xa_load(&pgmap_array, PHYS_PFN(phys));
	if (pgmap && !percpu_ref_tryget_live(pgmap->ref))
		pgmap = NULL;
	rcu_read_unlock();

	return pgmap;
}
EXPORT_SYMBOL_GPL(get_dev_pagemap);

#ifdef CONFIG_DEV_PAGEMAP_OPS
void free_devmap_managed_page(struct page *page)
{
	/* notify page idle for dax */
	if (!is_device_private_page(page)) {
		wake_up_var(&page->_refcount);
		return;
	}

	/* Clear Active bit in case of parallel mark_page_accessed */
	__ClearPageActive(page);
	__ClearPageWaiters(page);

	mem_cgroup_uncharge(page);

	/*
	 * When a device_private page is freed, the page->mapping field
	 * may still contain a (stale) mapping value. For example, the
	 * lower bits of page->mapping may still identify the page as an
	 * anonymous page. Ultimately, this entire field is just stale
	 * and wrong, and it will cause errors if not cleared.  One
	 * example is:
	 *
	 *  migrate_vma_pages()
	 *    migrate_vma_insert_page()
	 *      page_add_new_anon_rmap()
	 *        __page_set_anon_rmap()
	 *          ...checks page->mapping, via PageAnon(page) call,
	 *            and incorrectly concludes that the page is an
	 *            anonymous page. Therefore, it incorrectly,
	 *            silently fails to set up the new anon rmap.
	 *
	 * For other types of ZONE_DEVICE pages, migration is either
	 * handled differently or not done at all, so there is no need
	 * to clear page->mapping.
	 */
	page->mapping = NULL;
	page->pgmap->ops->page_free(page);
}
#endif /* CONFIG_DEV_PAGEMAP_OPS */
