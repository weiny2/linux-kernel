// SPDX-License-Identifier: GPL-2.0-only
/*
 * Copyright (C) 2020 Intel Corp
 */
#ifndef _DMATEST_H_
#define _DMATEST_H_

struct dmatest_data {
	u8		**raw;
	u8		**aligned;
	unsigned int	cnt;
	unsigned int	off;
};

struct dmatest_kdirect_ops {
	void *filter_param;
	int (*prep)(struct dma_chan *chan);
	void (*cleanup)(struct dma_chan *chan);
	int (*operation)(struct dma_chan *chan, struct dmatest_data *src,
			struct dmatest_data *dst, unsigned int len);
	bool (*dma_filter)(struct dma_chan *chan, void *filter_param);
};

extern int dmatest_kdirect_register(const struct dmatest_kdirect_ops *kdops);
extern void dmatest_kdirect_unregister(void);

#endif
