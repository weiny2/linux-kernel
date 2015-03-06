#ifndef __NFIT_TEST_H__
#define __NFIT_TEST_H__
#include <linux/types.h>

struct nfit_test_resource {
	struct list_head list;
	struct resource *res;
	void *buf;
};

typedef struct nfit_test_resource *(*nfit_test_lookup_fn)(resource_size_t);
struct nd_region;
typedef unsigned int (*nfit_test_acquire_lane_fn)(struct nd_region *nd_region);
typedef void (*nfit_test_release_lane_fn)(struct nd_region *nd_region,
		unsigned int lane);
struct nd_blk_window;
struct page;
typedef int (*nfit_test_blk_do_io_fn)(struct nd_blk_window *ndbw,
		struct page *page, unsigned int len, unsigned int off, int rw,
		resource_size_t dev_offset);
void nfit_test_setup(nfit_test_lookup_fn lookup,
		nfit_test_acquire_lane_fn acquire_lane,
		nfit_test_release_lane_fn release_lane,
		nfit_test_blk_do_io_fn blk_do_io);
void nfit_test_teardown(void);
#endif
