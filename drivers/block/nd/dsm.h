/*
 * Copyright(c) 2014 Intel Corporation. All rights reserved.
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

#ifndef __DSM_H__
#define __DSM_H__

#include "nfit.h"

#if IS_ENABLED(CONFIG_ND_MANUAL_DSM)
int nd_dsm_ctl(struct nfit_bus_descriptor *nfit_desc, unsigned int cmd,
		void *buf, unsigned int buf_len);

extern unsigned long nd_manual_dsm;
#else
static inline int nd_dsm_ctl(struct nfit_bus_descriptor *nfit_desc,
		unsigned int cmd, void *buf, unsigned int buf_len)
{
	return -ENOTTY;
}

static unsigned long nd_manual_dsm;
#endif

#endif /* __DSM_H__ */
