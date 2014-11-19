#include <linux/export.h>
#include <linux/ioport.h>
#include <linux/module.h>
#include <linux/types.h>
#include <linux/io.h>
#include "nfit_test.h"

static struct nfit_test_resource *null_lookup(resource_size_t offset)
{
	return NULL;
}

static nfit_test_lookup_fn nfit_test_lookup = null_lookup;
static DEFINE_SPINLOCK(nfit_test_lock);

void nfit_test_set_lookup_fn(nfit_test_lookup_fn fn)
{
	spin_lock(&nfit_test_lock);
	nfit_test_lookup = fn;
	spin_unlock(&nfit_test_lock);
}
EXPORT_SYMBOL(nfit_test_set_lookup_fn);

void nfit_test_clear_lookup_fn(void)
{
	spin_lock(&nfit_test_lock);
	nfit_test_lookup = null_lookup;
	spin_unlock(&nfit_test_lock);
}
EXPORT_SYMBOL(nfit_test_clear_lookup_fn);

void __iomem *__wrap_ioremap_cache(resource_size_t offset, unsigned long size)
{
	struct nfit_test_resource *nfit_res;

	spin_lock(&nfit_test_lock);
	nfit_res = nfit_test_lookup(offset);
	spin_unlock(&nfit_test_lock);
	if (nfit_res)
		return nfit_res->buf;
	return ioremap_cache(offset, size);
}
EXPORT_SYMBOL(__wrap_ioremap_cache);

void __wrap_iounmap(volatile void __iomem *addr)
{
	struct nfit_test_resource *nfit_res;

	spin_lock(&nfit_test_lock);
	nfit_res = nfit_test_lookup((resource_size_t) addr);
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

	spin_lock(&nfit_test_lock);
	nfit_res = nfit_test_lookup(start);
	spin_unlock(&nfit_test_lock);
	if (nfit_res) {
		struct resource *res;

		res = nfit_res->res;
		if (res)
			res->end = res->start + n;
		return res;
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
	spin_unlock(&nfit_test_lock);
	if (nfit_res)
		return;
	return __release_region(parent, start, n);
}
EXPORT_SYMBOL(__wrap___release_region);

MODULE_LICENSE("GPL v2");
