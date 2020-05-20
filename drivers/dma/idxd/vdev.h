/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright(c) 2019 Intel Corporation. All rights rsvd. */

#ifndef _IDXD_VDEV_H_
#define _IDXD_VDEV_H_

static inline u64 get_reg_val(void *buf, int size)
{
	u64 val = 0;

	switch (size) {
	case 8:
		val = *(uint64_t *)buf;
		break;
	case 4:
		val = *(uint32_t *)buf;
		break;
	case 2:
		val = *(uint16_t *)buf;
		break;
	case 1:
		val = *(uint8_t *)buf;
		break;
	}

	return val;
}

static inline u8 vidxd_state(struct vdcm_idxd *vidxd)
{
	return vidxd->bar0.cap_ctrl_regs[IDXD_GENSTATS_OFFSET]
		& IDXD_GENSTATS_MASK;
}

void vidxd_mmio_init(struct vdcm_idxd *vidxd);
int vidxd_free_ims_entry(struct vdcm_idxd *vidxd, int msix_idx);
int vidxd_setup_ims_entry(struct vdcm_idxd *vidxd, int ims_idx, u32 val);
int vidxd_send_interrupt(struct vdcm_idxd *vidxd, int msix_idx);
void vidxd_do_command(struct vdcm_idxd *vidxd, u32 val);
void vidxd_reset(struct vdcm_idxd *vidxd);

#endif
