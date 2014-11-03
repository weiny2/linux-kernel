#ifndef __NFIT_TEST_H__
#define __NFIT_TEST_H__

struct nfit_test_resource {
	struct list_head list;
	struct resource *res;
	void *buf;
};

extern struct nfit_test_resource *(*nfit_test_lookup)(resource_size_t);
#endif
