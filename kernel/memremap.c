/*
 * Copyright(c) 2015 Intel Corporation. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of version 2 of the GNU General Public License as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 */
#include <linux/rculist.h>
#include <linux/device.h>
#include <linux/types.h>
#include <linux/fs.h>
#include <linux/io.h>
#include <linux/mm.h>
#include <linux/memory_hotplug.h>

__weak void *arch_memremap(resource_size_t offset, size_t size,
		unsigned long flags)
{
	if (!IS_ENABLED(CONFIG_MMU))
		return (void *) (unsigned long) offset;
	WARN_ONCE(1, "%s in %s should only be called in NOMMU configurations\n",
			__func__, __FILE__);
	return NULL;
}

/**
 * memremap() - remap an iomem_resource as cacheable memory
 * @offset: iomem resource start address
 * @size: size of remap
 * @flags: either MEMREMAP_WB or MEMREMAP_WT
 *
 * memremap() is "ioremap" for cases where it is known that the resource
 * being mapped does not have i/o side effects and the __iomem
 * annotation is not applicable.
 *
 * MEMREMAP_WB - matches the default mapping for "System RAM" on
 * the architecture.  This is usually a read-allocate write-back cache.
 * Morever, if MEMREMAP_WB is specified and the requested remap region is RAM
 * memremap() will bypass establishing a new mapping and instead return
 * a pointer into the direct map.
 *
 * MEMREMAP_WT - establish a mapping whereby writes either bypass the
 * cache or are written through to memory and never exist in a
 * cache-dirty state with respect to program visibility.  Attempts to
 * map "System RAM" with this mapping type will fail.
 */
void *memremap(resource_size_t offset, size_t size, unsigned long flags)
{
	int is_ram = region_intersects(offset, size, "System RAM");

	if (is_ram == REGION_MIXED) {
		WARN_ONCE(1, "memremap attempted on mixed range %pa size: %#lx\n",
				&offset, (unsigned long) size);
		return NULL;
	}

	/* Try all mapping types requested until one returns non-NULL */
	if (flags & MEMREMAP_WB) {
		/*
		 * MEMREMAP_WB is special in that it can be satisifed
		 * from the direct map.  Some archs depend on the
		 * capability of memremap() to autodetect cases where
		 * the requested range is potentially in "System RAM"
		 */
		if (is_ram == REGION_INTERSECTS)
			return __va(offset);
	}

	/*
	 * If we don't have a mapping yet and more request flags are
	 * pending then we will be attempting to establish a new virtual
	 * address mapping.  Enforce that this mapping is not aliasing
	 * "System RAM"
	 */
	if (is_ram == REGION_INTERSECTS) {
		WARN_ONCE(1, "memremap attempted on ram %pa size: %#lx\n",
				&offset, (unsigned long) size);
		return NULL;
	}

	return arch_memremap(offset, size, flags);
}
EXPORT_SYMBOL(memremap);

void memunmap(void *addr)
{
	if (is_vmalloc_addr(addr))
		iounmap((void __iomem *) addr);
}
EXPORT_SYMBOL(memunmap);

static void devm_memremap_release(struct device *dev, void *res)
{
	memunmap(res);
}

static int devm_memremap_match(struct device *dev, void *res, void *match_data)
{
	return *(void **)res == match_data;
}

void *devm_memremap(struct device *dev, resource_size_t offset,
		size_t size, unsigned long flags)
{
	void **ptr, *addr;

	ptr = devres_alloc_node(devm_memremap_release, sizeof(*ptr), GFP_KERNEL,
			dev_to_node(dev));
	if (!ptr)
		return ERR_PTR(-ENOMEM);

	addr = memremap(offset, size, flags);
	if (addr) {
		*ptr = addr;
		devres_add(dev, ptr);
	} else
		devres_free(ptr);

	return addr;
}
EXPORT_SYMBOL(devm_memremap);

void devm_memunmap(struct device *dev, void *addr)
{
	WARN_ON(devres_release(dev, devm_memremap_release,
				devm_memremap_match, addr));
}
EXPORT_SYMBOL(devm_memunmap);

#ifdef CONFIG_ZONE_DEVICE
static LIST_HEAD(ranges);
static DEFINE_SPINLOCK(range_lock);

struct page_map {
	struct resource res;
	struct dev_pagemap pgmap;
	struct list_head list;
	struct vmem_altmap altmap;
};

static void add_page_map(struct page_map *page_map)
{
	spin_lock(&range_lock);
	list_add_rcu(&page_map->list, &ranges);
	spin_unlock(&range_lock);
}

static void del_page_map(struct page_map *page_map)
{
	spin_lock(&range_lock);
	list_del_rcu(&page_map->list);
	spin_unlock(&range_lock);
}

static unsigned long pfn_first(struct dev_pagemap *pgmap)
{
	struct vmem_altmap *altmap = pgmap->altmap;
	const struct resource *res = pgmap->res;
	unsigned long pfn;

	pfn = res->start >> PAGE_SHIFT;
	if (altmap)
		pfn += vmem_altmap_offset(altmap);
	return pfn;
}

static unsigned long pfn_end(struct dev_pagemap *pgmap)
{
	const struct resource *res = pgmap->res;

	return (res->start + resource_size(res)) >> PAGE_SHIFT;
}

#define for_each_device_pfn(pfn, pgmap) \
	for (pfn = pfn_first(pgmap); pfn < pfn_end(pgmap); pfn++)

static void devm_memremap_pages_release(struct device *dev, void *data)
{
	unsigned long pfn;
	struct page_map *page_map = data;
	struct resource *res = &page_map->res;
	struct address_space *mapping_prev = NULL;
	struct dev_pagemap *pgmap = &page_map->pgmap;

	if (percpu_ref_tryget_live(pgmap->ref)) {
		dev_WARN(dev, "%s: page mapping is still live!\n", __func__);
		percpu_ref_put(pgmap->ref);
	}

	/* flush in-flight dax_map_atomic() operations */
	synchronize_rcu();

	for_each_device_pfn(pfn, pgmap) {
		struct page *page = pfn_to_page(pfn);
		struct address_space *mapping = page->mapping;
		struct inode *inode = mapping ? mapping->host : NULL;

		dev_WARN_ONCE(dev, atomic_read(&page->_count) < 1,
				"%s: ZONE_DEVICE page was freed!\n", __func__);

		if (!mapping || !inode || mapping == mapping_prev) {
			dev_WARN_ONCE(dev, atomic_read(&page->_count) > 1,
					"%s: unexpected elevated page count pfn: %lx\n",
					__func__, pfn);
			continue;
		}

		truncate_pagecache(inode, 0);
		mapping_prev = mapping;
	}

	/* pages are dead and unused, undo the arch mapping */
	arch_remove_memory(res->start, resource_size(res));
	dev_WARN_ONCE(dev, pgmap->altmap && pgmap->altmap->alloc,
			"%s: failed to free all reserved pages\n", __func__);
	del_page_map(page_map);
}

/* assumes rcu_read_lock() held at entry */
struct dev_pagemap *__get_dev_pagemap(resource_size_t phys)
{
	struct page_map *page_map;

	WARN_ON_ONCE(!rcu_read_lock_held());

	list_for_each_entry_rcu(page_map, &ranges, list)
		if (phys >= page_map->res.start && phys <= page_map->res.end)
			return &page_map->pgmap;
	return NULL;
}

/**
 * devm_memremap_pages - remap and provide memmap backing for the given resource
 * @dev: hosting device for @res
 * @res: "host memory" address range
 * @ref: a live per-cpu reference count
 * @altmap: optional descriptor for allocating the memmap from @res
 *
 * Notes:
 * 1/ @ref must be 'live' on entry and 'dead' before devm_memunmap_pages() time
 *    (or devm release event).
 *
 * 2/ @res is expected to be a host memory range that could feasibly be
 *    treated as a "System RAM" range, i.e. not a device mmio range, but
 *    this is not enforced.
 */
void *devm_memremap_pages(struct device *dev, struct resource *res,
		struct percpu_ref *ref, struct vmem_altmap *altmap)
{
	int is_ram = region_intersects(res->start, resource_size(res),
			"System RAM");
	struct dev_pagemap *pgmap;
	struct page_map *page_map;
	unsigned long pfn;
	int error, nid;

	if (is_ram == REGION_MIXED) {
		WARN_ONCE(1, "%s attempted on mixed region %pr\n",
				__func__, res);
		return ERR_PTR(-ENXIO);
	}

	if (is_ram == REGION_INTERSECTS)
		return __va(res->start);

	if (!ref)
		return ERR_PTR(-EINVAL);

	page_map = devres_alloc_node(devm_memremap_pages_release,
			sizeof(*page_map), GFP_KERNEL, dev_to_node(dev));
	if (!page_map)
		return ERR_PTR(-ENOMEM);
	pgmap = &page_map->pgmap;

	memcpy(&page_map->res, res, sizeof(*res));
	if (altmap) {
		memcpy(&page_map->altmap, altmap, sizeof(*altmap));
		pgmap->altmap = &page_map->altmap;
	}
	pgmap->dev = dev;
	pgmap->ref = ref;
	pgmap->res = &page_map->res;
	INIT_LIST_HEAD(&page_map->list);
	add_page_map(page_map);

	nid = dev_to_node(dev);
	if (nid < 0)
		nid = numa_mem_id();

	error = arch_add_memory(nid, res->start, resource_size(res), true);
	if (error) {
		del_page_map(page_map);
		devres_free(page_map);
		return ERR_PTR(error);
	}

	for_each_device_pfn(pfn, pgmap) {
		struct page *page = pfn_to_page(pfn);

		list_del_poison(&page->lru);
		page->pgmap = pgmap;
	}
	devres_add(dev, page_map);
	return __va(res->start);
}
EXPORT_SYMBOL(devm_memremap_pages);

static int page_map_match(struct device *dev, void *res, void *match_data)
{
	struct page_map *page_map = res;
	resource_size_t phys = *(resource_size_t *) match_data;

	return page_map->res.start == phys;
}

void devm_memunmap_pages(struct device *dev, void *addr)
{
	resource_size_t start = __pa(addr);

	if (devres_release(dev, devm_memremap_pages_release, page_map_match,
				&start) != 0)
		dev_WARN(dev, "failed to find page map to release\n");
}
EXPORT_SYMBOL(devm_memunmap_pages);

/*
 * Uncoditionally retrieve a dev_pagemap associated with the given physical
 * address, this is only for use in the arch_{add|remove}_memory() for setting
 * up and tearing down the memmap.
 */
static struct dev_pagemap *lookup_dev_pagemap(resource_size_t phys)
{
	struct dev_pagemap *pgmap;

	rcu_read_lock();
	pgmap = __get_dev_pagemap(phys);
	rcu_read_unlock();
	return pgmap;
}

struct vmem_altmap *to_vmem_altmap(unsigned long memmap_start)
{
	/*
	 * 'memmap_start' is the virtual address for the first "struct
	 * page" in this range of the vmemmap array.  In the case of
	 * CONFIG_SPARSE_VMEMMAP a page_to_pfn conversion is simple
	 * pointer arithmetic, so we can perform this to_vmem_altmap()
	 * conversion without concern for the initialization state of
	 * the struct page fields.
	 */
	struct page *page = (struct page *) memmap_start;
	struct dev_pagemap *pgmap;

	pgmap = lookup_dev_pagemap(__pfn_to_phys(page_to_pfn(page)));
	return pgmap ? pgmap->altmap : NULL;
}
#endif /* CONFIG_ZONE_DEVICE */
