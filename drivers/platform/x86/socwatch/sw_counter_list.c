/* SPDX-License-Identifier: GPL-2.0 AND BSD-3-Clause
 *
 * This file is provided under a dual BSD/GPLv2 license.  When using or
 * redistributing this file, you may do so under either license.
 *
 * GPL LICENSE SUMMARY
 *
 * Copyright(c) 2020 Intel Corporation.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of version 2 of the GNU General Public License as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * Contact Information:
 * SoC Watch Developer Team <socwatchdevelopers@intel.com>
 * Intel Corporation,
 * 1300 S Mopac Expwy,
 * Austin, TX 78746
 *
 * BSD LICENSE
 *
 * Copyright(c) 2020 Intel Corporation.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 *   * Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 *   * Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in
 *     the documentation and/or other materials provided with the
 *     distribution.
 *   * Neither the name of Intel Corporation nor the names of its
 *     contributors may be used to endorse or promote products derived
 *     from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "sw_internal.h"

#include "sw_counter_list.h"
#include "sw_counter_info.h"

static pw_u8_t *msr_search_array = NULL;
static pw_u64_t msr_search_array_size = 0;

static pw_u64_t get_msr_search_array_size_i(void) {

	if (msr_search_array) {
		return msr_search_array_size;
	} else {
		return 0;
	}
}

static int sw_init_msr_search_array_i(void) {

	pw_u64_t i = 0, max_msr_value = 0;//, msr_array_size = 0;
	pw_u64_t msr_list_size = sizeof(msr_info_list) / sizeof(msr_info_list[0]);

	// TODO: Probably sort msr_info_list rather than assuming it is sorted.

	// Since 'msr_info_list' is sorted, the last entry should be the highest MSR
	// address
	max_msr_value = msr_info_list[msr_list_size-1];
	pw_pr_debug("max msr value: %llx\n", max_msr_value);

	// TODO: Optimize the memory usage by making msr_search_array a bit vector
	msr_search_array_size = max_msr_value + 1;
	msr_search_array = sw_kmalloc(msr_search_array_size * sizeof(pw_u8_t),
			GFP_KERNEL);

	if (msr_search_array == NULL) {
		return -PW_ERROR;
	}

	memset(msr_search_array, 0, msr_search_array_size);

	for (i = 0; i < msr_list_size; ++i) {
		msr_search_array[msr_info_list[i]] = 1;
	}

	return PW_SUCCESS;
}

int sw_counter_init_search_lists(void) {

	return sw_init_msr_search_array_i();
}

static void sw_destroy_msr_search_array_i(void) {

	if (msr_search_array == NULL) {
		return;
	}

	sw_kfree(msr_search_array);
	msr_search_array = NULL;
	msr_search_array_size = 0;
}

void sw_counter_destroy_search_lists(void) {

	sw_destroy_msr_search_array_i();
}

bool sw_counter_is_valid_msr(pw_u64_t msr_id) {

	if (msr_id >= get_msr_search_array_size_i()) {
		return false;
	}
	return msr_search_array[msr_id];
}
