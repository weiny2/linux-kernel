#include <linux/export.h>
#include <linux/module.h>
#include <linux/types.h>

void __iomem *(*nfit_test_ioremap)(resource_size_t, unsigned long);
EXPORT_SYMBOL(nfit_test_ioremap);

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

MODULE_LICENSE("Dual BSD/GPL");
