#include <linux/export.h>
#include <linux/module.h>
#include <linux/types.h>

void __iomem *(*nfit_test_ioremap)(resource_size_t, unsigned long);
EXPORT_SYMBOL(nfit_test_ioremap);

struct resource *(*nfit_test_request_region)(resource_size_t start,
		resource_size_t n);
EXPORT_SYMBOL(nfit_test_request_region);

void __iomem *__wrap_ioremap_cache(resource_size_t offset, unsigned long size)
{
	if (nfit_test_ioremap)
		return nfit_test_ioremap(offset, size);
	return NULL;
}
EXPORT_SYMBOL(__wrap_ioremap_cache);

extern void __wrap_iounmap(volatile void __iomem *addr)
{
}
EXPORT_SYMBOL(__wrap_iounmap);

struct resource * __wrap___request_region(struct resource *parent,
		resource_size_t start, resource_size_t n, const char *name,
		int flags)
{
	if (nfit_test_request_region)
		return nfit_test_request_region(start, n);
	return NULL;
}
EXPORT_SYMBOL(__wrap___request_region);

void __wrap___release_region(struct resource *parent, resource_size_t start,
				resource_size_t n)
{
}
EXPORT_SYMBOL(__wrap___release_region);

MODULE_LICENSE("GPL v2");
