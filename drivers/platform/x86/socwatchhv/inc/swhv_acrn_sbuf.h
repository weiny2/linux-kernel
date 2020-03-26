/* SPDX-License-Identifier: GPL-2.0 AND BSD-3-Clause
 *
 * This file is provided under a dual BSD/GPLv2 license.  When using or
 * redistributing this file, you may do so under either license.
 *
 * GPL LICENSE SUMMARY
 *
 * Copyright(c) 2014 - 2019 Intel Corporation.
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
 * Copyright(c) 2014 - 2019 Intel Corporation.
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

#ifndef _SWHV_ACRN_SBUF_H_
#define _SWHV_ACRN_SBUF_H_ 1

#include <acrn/sbuf.h>

/*
 * Checks if the passed sbuf is empty.
 */
static inline bool sbuf_is_empty(struct shared_buf *sbuf)
{
    return (sbuf->head == sbuf->tail);
}

static inline uint32_t sbuf_next_ptr(uint32_t pos,
	uint32_t span, uint32_t scope)
{
	pos += span;
	pos = (pos >= scope) ? (pos - scope) : pos;
	return pos;
}

/*
 * This function returns the available free space in the
 * passed sbuf.
 */
inline uint32_t sbuf_available_space(struct shared_buf *sbuf)
{
	uint32_t remaining_space;
	/*
	 * if tail isn't wrapped around
	 *	  subtract difference of tail and head from size
	 * otherwise
	 *	  difference between head and tail
	 */
	if (sbuf->tail >= sbuf->head)
		remaining_space = sbuf->size - (sbuf->tail - sbuf->head);
	else
		remaining_space = sbuf->head - sbuf->tail;

	return remaining_space;
}

/*
 * This function retrieves the requested 'size' amount of data from
 * the passed buffer.
 * This is a much more efficient implementation than the default
 * 'sbuf_get()' which retrieves one 'element' size at a time.
 */
int sbuf_get_variable(struct shared_buf *sbuf, void **data, uint32_t size)
{
	/*
	 * 1. Check if buffer isn't empty and non-zero 'size'
	 * 2. check if enough ('size' bytes) data to be read is present.
	 * 3. Continue if buffer has enough data
	 * 4. Copy data from buffer
	 *	  4a. copy data in 2 parts if there is a wrap-around
	 *	  4b. Otherwise do a simple copy
	 */
	const void *from;
	uint32_t current_data_size, offset = 0, next_head;

	if ((sbuf == NULL) || (*data == NULL))
		return -EINVAL;

	if (sbuf_is_empty(sbuf) || (size == 0)) {
		/* no data available */
		return 0;
	}

	current_data_size = sbuf->size - sbuf_available_space(sbuf);

	/*
	 * TODO If requested data size is greater than current buffer size,
	 * consider at least copying the current buffer size.
	 */
	if (size > current_data_size) {
		pw_pr_warn(
			"Requested data size is greater than the current buffer size!");
		/* not enough data to be read */
		return 0;
	}

	next_head = sbuf_next_ptr(sbuf->head, size, sbuf->size);

	from = (void *)sbuf + SBUF_HEAD_SIZE + sbuf->head;

	if (next_head < sbuf->head) { /* wrap-around */
		/* copy first part */
		offset = sbuf->size - sbuf->head;
		memcpy(*data, from, offset);

		from = (void *)sbuf + SBUF_HEAD_SIZE;
	}
	memcpy((void *)*data + offset, from, size - offset);

	sbuf->head = next_head;

	return size;
}

/*
 * This API can be used to retrieve complete samples at a time from the
 * sbuf. It internally uses the sbuf_get() which retrieves 1 'element'
 * at a time and is probably not very efficient for reading large amount
 * of data.
 * Note: Not used currently.
 */
int sbuf_get_wrapper(struct shared_buf *sbuf, uint8_t **data)
{
	uint8_t *sample;
	uint8_t sample_offset;
	acrn_msg_header *header;
	uint32_t payload_size, sample_size, _size;

	/*
	 * Assumption: A partial variable sample will not be written to
	 *	  the buffer.
	 * do while buf isn't empty
	 *  Read header from the buffer
	 *	  write to data
	 *	  get size of payload
	 *	  check if the size of 'data' is enough for the variable
	 *	  sample to be read to
	 *  Read the payload
	 *	  Keep reading ele_size chunks till available and write to data
	 *	  if the last chunk is less than ele_size, do a partial
	 *	  copy to data
	 *
	 *
	 */
	if ((sbuf == NULL) || (data == NULL))
		return -EINVAL;

	if (sbuf_is_empty(sbuf)) /* no data available */
		return 0;


	sample_offset = 0;

	header = vmalloc(sizeof(ACRN_MSG_HEADER_SIZE));
	memset(header, 0, sizeof(ACRN_MSG_HEADER_SIZE));
	/* read header */
	sbuf_get(sbuf, (uint8_t *)header);

	payload_size = header->payload_size;

	sample_size = ACRN_MSG_HEADER_SIZE + header->payload_size;

	sample = vmalloc(sample_size);

	/* copy header */
	memcpy((void *)sample, (void *)header, ACRN_MSG_HEADER_SIZE);

	sample_offset += ACRN_MSG_HEADER_SIZE;

	_size = payload_size;
	while (_size) {
		if (_size >= sbuf->ele_size) {
			sbuf_get(sbuf, (uint8_t *)(sample + sample_offset));
			sample_offset += sbuf->ele_size;
			_size -= sbuf->ele_size;
		} else {
			pw_pr_error(
				"error: payload has to be multiple of 32\n");
			return 0;
			/*
			 * This code can be enabled when support for
			 * variable sized samples needs to be added.
			 */
/* #if 0
			chunk = malloc(sbuf->ele_size);
			sbuf_get(sbuf, chunk);
			memcpys((void *)(sample + sample_offset),
					_size, chunk);
			_size -= _size;
			free(chunk);
#endif */
		}
	}

	*data = sample;

	vfree(header);
	return sample_size;
}
#endif /* _SWHV_ACRN_SBUF_H_ */
