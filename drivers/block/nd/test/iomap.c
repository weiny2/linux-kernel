#include <linux/export.h>
#include <linux/ioport.h>
#include <linux/module.h>
#include <linux/types.h>
#include <linux/io.h>
#include "nfit_test.h"

struct nfit_test_resource *(*nfit_test_lookup)(resource_size_t);
EXPORT_SYMBOL(nfit_test_lookup);

void __iomem *__wrap_ioremap_cache(resource_size_t offset, unsigned long size)
{
	struct nfit_test_resource *nfit_res;

	nfit_res = nfit_test_lookup ? nfit_test_lookup(offset) : NULL;
	if (nfit_res)
		return nfit_res->buf;
	return ioremap_cache(offset, size);
}
EXPORT_SYMBOL(__wrap_ioremap_cache);

void __wrap_iounmap(volatile void __iomem *addr)
{
	struct nfit_test_resource *nfit_res;

	nfit_res = nfit_test_lookup ?
		nfit_test_lookup((resource_size_t) addr) : NULL;
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

	nfit_res = nfit_test_lookup ? nfit_test_lookup(start) : NULL;
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

	nfit_res = nfit_test_lookup ? nfit_test_lookup(start) : NULL;
	if (nfit_res)
		return;
	return __release_region(parent, start, n);
}
EXPORT_SYMBOL(__wrap___release_region);

MODULE_LICENSE("GPL v2");
