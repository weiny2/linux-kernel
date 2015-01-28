/*
 * Copyright (c) 2014 Intel Corporation. All rights reserved.
 * Copyright (c) 2006, 2007, 2008, 2009, 2010, 2011 QLogic Corporation.
 * All rights reserved.
 * Copyright (c) 2003, 2004, 2005, 2006 PathScale, Inc. All rights reserved.
 *
 * This software is available to you under a choice of one of two
 * licenses.  You may choose to be licensed under the terms of the GNU
 * General Public License (GPL) Version 2, available from the file
 * COPYING in the main directory of this source tree, or the
 * OpenIB.org BSD license below:
 *
 *     Redistribution and use in source and binary forms, with or
 *     without modification, are permitted provided that the following
 *     conditions are met:
 *
 *      - Redistributions of source code must retain the above
 *        copyright notice, this list of conditions and the following
 *        disclaimer.
 *
 *      - Redistributions in binary form must reproduce the above
 *        copyright notice, this list of conditions and the following
 *        disclaimer in the documentation and/or other materials
 *        provided with the distribution.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS
 * BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN
 * ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

/*
 * This file is conditionally built on x86_64 only.  Otherwise weak symbol
 * versions of the functions exported from here are used.
 */

#include <linux/pci.h>
#include <linux/module.h>
#include <asm/mtrr.h>
#include <asm/processor.h>

#include "qib.h"

static int use_string_ops;

static unsigned qib_ss_size;
module_param_named(ss_size, qib_ss_size, uint, S_IRUGO);
MODULE_PARM_DESC(ss_size,
	"Threshold to use streaming store (default is 512K)");

static unsigned qib_cache_bypass_copy;
module_param_named(cache_bypass_copy, qib_cache_bypass_copy, uint, S_IRUGO);
MODULE_PARM_DESC(cache_bypass_copy, "Use cache bypass copies");

void *memcpy_cachebypass(void *, const void *, __kernel_size_t);
void *memcpy_cachebypass2(void *, const void *, __kernel_size_t);

/**
 * qib_copy_sge - copy data to SGE memory
 * @ss: the SGE state
 * @data: the data to copy
 * @length: the length of the data
 */
void qib_copy_sge(struct qib_sge_state *ss, void *data, u32 length, int release)
{
	struct qib_sge *sge = &ss->sge;

	while (length) {
		u32 len;
		len = sge->length;

		if (len > length)
			len = length;
		if (len > sge->sge_length)
			len = sge->sge_length;
		BUG_ON(len == 0);
		if (!qib_cache_bypass_copy) {
			if (use_string_ops)
				memcpy_string_op(sge->vaddr, data, len);
			else {
				if (len < 64)
					memcpy(sge->vaddr, data, len);
				else
					__memcpy(sge->vaddr, data, len);
			}
		} else {
			if ((!sge->mr->pd || !to_ipd(sge->mr->pd)->user)
					&& ss->total_len >= qib_ss_size)
				memcpy_cachebypass2(sge->vaddr, data, len);
			else
				memcpy_cachebypass(sge->vaddr, data, len);
		}
		sge->vaddr += len;
		sge->length -= len;
		sge->sge_length -= len;
		if (sge->sge_length == 0) {
			if (release)
				atomic_dec(&sge->mr->refcount);
			if (--ss->num_sge)
				*sge = *ss->sg_list++;
		} else if (sge->length == 0 && sge->mr->lkey) {
			if (++sge->n >= QIB_SEGSZ) {
				if (++sge->m >= sge->mr->mapsz)
					break;
				sge->n = 0;
			}
			sge->vaddr =
				sge->mr->map[sge->m]->segs[sge->n].vaddr;
			sge->length =
				sge->mr->map[sge->m]->segs[sge->n].length;
		}
		data += len;
		length -= len;
	}
}

void qib_copy_sge_init(void)
{
	if (!qib_cache_bypass_copy) {
		if (boot_cpu_has(X86_FEATURE_REP_GOOD)) {
			use_string_ops = 1;
		} else {
			if (boot_cpu_data.x86_vendor == X86_VENDOR_INTEL &&
					boot_cpu_data.x86 == 6)
				use_string_ops = 1;
			if (boot_cpu_data.x86_vendor == X86_VENDOR_AMD) {
				if (boot_cpu_data.x86 == 0xf) {
					u32 level;
					level = cpuid_eax(1);
					if ((level >= 0x0f48
					    && level < 0x0f50) ||
					    level >= 0x0f58)
						use_string_ops = 1;
				} else
					if (boot_cpu_data.x86 >= 0x10)
						use_string_ops = 1;
			}
		}
		if (boot_cpu_data.x86_vendor == X86_VENDOR_AMD)
			qib_cache_bypass_copy = 1;
	}
	if (!qib_ss_size)
		qib_ss_size = 512 * 1024;
}
