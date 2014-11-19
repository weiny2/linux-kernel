#ifndef __NFIT_TEST_H__
#define __NFIT_TEST_H__

struct nfit_test_resource {
	struct list_head list;
	struct resource *res;
	void *buf;
};

typedef struct nfit_test_resource *(*nfit_test_lookup_fn)(resource_size_t);
void nfit_test_set_lookup_fn(nfit_test_lookup_fn fn);
void nfit_test_clear_lookup_fn(void);
#endif
