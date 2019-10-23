/* SPDX-License-Identifier: GPL-2.0 */
#ifndef _INTEL_CTA_TELEM_H
#define _INTEL_CTA_TELEM_H

#include <linux/intel-dvsec.h>

/* Telemetry types */
#define CTA_TELEM_TELEMETRY	0
#define CTA_TELEM_CRASHLOG	1

struct telem_header {
	u8	access_type;
	u8	telem_type;
	u16	size;
	u32	guid;
	u32	base_offset;
	u8	tbir;
};

#endif
