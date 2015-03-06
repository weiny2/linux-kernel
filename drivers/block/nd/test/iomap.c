#include <linux/highmem.h>
#include <linux/export.h>
#include <linux/ioport.h>
#include <linux/module.h>
#include <linux/types.h>
#include <linux/io.h>
#include "nfit_test.h"
#include "../nd.h"

static struct nfit_test_resource *null_lookup(resource_size_t offset)
{
	return NULL;
}

static nfit_test_lookup_fn nfit_test_lookup = null_lookup;
static nfit_test_acquire_lane_fn nfit_test_acquire_lane;
static nfit_test_release_lane_fn nfit_test_release_lane;
static nfit_test_blk_do_io_fn nfit_test_blk_do_io;

static DEFINE_SPINLOCK(nfit_test_lock);

void nfit_test_setup(nfit_test_lookup_fn lookup,
		nfit_test_acquire_lane_fn acquire_lane,
		nfit_test_release_lane_fn release_lane,
		nfit_test_blk_do_io_fn blk_do_io)
{
	spin_lock(&nfit_test_lock);
	nfit_test_lookup = lookup;
	nfit_test_acquire_lane = acquire_lane;
	nfit_test_release_lane = release_lane;
	nfit_test_blk_do_io = blk_do_io;
	spin_unlock(&nfit_test_lock);
}
EXPORT_SYMBOL(nfit_test_setup);

void nfit_test_teardown(void)
{
	spin_lock(&nfit_test_lock);
	nfit_test_lookup = null_lookup;
	nfit_test_acquire_lane = NULL;
	nfit_test_release_lane = NULL;
	spin_unlock(&nfit_test_lock);
}
EXPORT_SYMBOL(nfit_test_teardown);

void __iomem *__wrap_ioremap_cache(resource_size_t offset, unsigned long size)
{
	struct nfit_test_resource *nfit_res;

	spin_lock(&nfit_test_lock);
	nfit_res = nfit_test_lookup(offset);
	spin_unlock(&nfit_test_lock);
	if (nfit_res)
		return (void __iomem *) nfit_res->buf + offset
			- nfit_res->res->start;
	return ioremap_cache(offset, size);
}
EXPORT_SYMBOL(__wrap_ioremap_cache);

void __wrap_iounmap(volatile void __iomem *addr)
{
	struct nfit_test_resource *nfit_res;

	spin_lock(&nfit_test_lock);
	nfit_res = nfit_test_lookup((unsigned long) addr);
	spin_unlock(&nfit_test_lock);
	if (nfit_res)
		return;
	return iounmap(addr);
}
EXPORT_SYMBOL(__wrap_iounmap);

struct resource * __wrap___request_region(struct resource *parent,
		resource_size_t start, resource_size_t n, const char *name,
		int flags)
{
	struct nfit_test_resource *nfit_res;

	if (parent == &iomem_resource) {
		spin_lock(&nfit_test_lock);
		nfit_res = nfit_test_lookup(start);
		spin_unlock(&nfit_test_lock);
		if (nfit_res) {
			struct resource *res = nfit_res->res + 1;

			if (start + n > nfit_res->res->start
					+ resource_size(nfit_res->res)) {
				pr_debug("%s: start: %llx n: %llx overflow: %pr\n",
						__func__, start, n, nfit_res->res);
				return NULL;
			}

			res->start = start;
			res->end = start + n - 1;
			res->name = name;
			res->flags = resource_type(parent);
			res->flags |= IORESOURCE_BUSY | flags;
			pr_debug("%s: %pr\n", __func__, res);
			return res;
		}
	}
	return __request_region(parent, start, n, name, flags);
}
EXPORT_SYMBOL(__wrap___request_region);

void __wrap___release_region(struct resource *parent, resource_size_t start,
				resource_size_t n)
{
	struct nfit_test_resource *nfit_res;

	spin_lock(&nfit_test_lock);
	nfit_res = nfit_test_lookup(start);
	if (nfit_res) {
		struct resource *res = nfit_res->res + 1;

		if (start != res->start || resource_size(res) != n)
			pr_info("%s: start: %llx n: %llx mismatch: %pr\n",
					__func__, start, n, res);
		else
			memset(res, 0, sizeof(*res));
	}
	spin_unlock(&nfit_test_lock);
	if (!nfit_res)
		__release_region(parent, start, n);
}
EXPORT_SYMBOL(__wrap___release_region);

void __iomem *__wrap_devm_ioremap_resource(struct device *dev,
		struct resource *res)
{
	struct nfit_test_resource *nfit_res;

	spin_lock(&nfit_test_lock);
	nfit_res = nfit_test_lookup(res->start);
	spin_unlock(&nfit_test_lock);
	if (nfit_res) {
		dev_dbg(dev, "%s: %pa -> %pa (%p)\n", __func__, &res->start,
				&nfit_res->res->start, nfit_res->buf);
		return (void __iomem *) nfit_res->buf + res->start
			- nfit_res->res->start;
	}
	return devm_ioremap_resource(dev, res);
}
EXPORT_SYMBOL(__wrap_devm_ioremap_resource);

int __wrap_nd_blk_do_io(struct nd_blk_window *ndbw, struct page *page,
		unsigned int len, unsigned int off, int rw,
		resource_size_t dev_offset)
{
	struct nd_region *nd_region = ndbw_to_region(ndbw);
	struct nfit_test_resource *nfit_res;

	spin_lock(&nfit_test_lock);
	nfit_res = nfit_test_lookup((unsigned long) ndbw->aperture);
	if (nfit_res) {
		void *mem = kmap_atomic(page);
		unsigned int bw;

		dev_vdbg(&nd_region->dev, "%s: base: %p offset: %pa\n",
				__func__, ndbw->aperture, &dev_offset);
		bw = nfit_test_acquire_lane(nd_region);
		if (rw)
			memcpy(nfit_res->buf + dev_offset, mem + off, len);
		else
			memcpy(mem + off, nfit_res->buf + dev_offset, len);
		nfit_test_release_lane(nd_region, bw);
		kunmap_atomic(mem);
	}
	spin_unlock(&nfit_test_lock);
	if (!nfit_res)
		return nfit_test_blk_do_io(ndbw, page, len, off, rw,
				dev_offset);
	return 0;
}
EXPORT_SYMBOL(__wrap_nd_blk_do_io);

MODULE_LICENSE("GPL v2");
