/*
 * Generic SCSI-3 ALUA SCSI Device Handler
 *
 * Copyright (C) 2007-2010 Hannes Reinecke, SUSE Linux Products GmbH.
 * All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 *
 */
#include <linux/slab.h>
#include <linux/delay.h>
#include <linux/module.h>
#include <linux/workqueue.h>
#include <linux/rcupdate.h>
#include <scsi/scsi.h>
#include <scsi/scsi_dbg.h>
#include <scsi/scsi_eh.h>
#include <scsi/scsi_dh.h>

#define ALUA_DH_NAME "alua"
#define ALUA_DH_VER "1.3"

#define TPGS_STATE_OPTIMIZED		0x0
#define TPGS_STATE_NONOPTIMIZED		0x1
#define TPGS_STATE_STANDBY		0x2
#define TPGS_STATE_UNAVAILABLE		0x3
#define TPGS_STATE_LBA_DEPENDENT	0x4
#define TPGS_STATE_OFFLINE		0xe
#define TPGS_STATE_TRANSITIONING	0xf

#define TPGS_SUPPORT_NONE		0x00
#define TPGS_SUPPORT_OPTIMIZED		0x01
#define TPGS_SUPPORT_NONOPTIMIZED	0x02
#define TPGS_SUPPORT_STANDBY		0x04
#define TPGS_SUPPORT_UNAVAILABLE	0x08
#define TPGS_SUPPORT_LBA_DEPENDENT	0x10
#define TPGS_SUPPORT_OFFLINE		0x40
#define TPGS_SUPPORT_TRANSITION		0x80

#define RTPG_FMT_MASK			0x70
#define RTPG_FMT_EXT_HDR		0x10

#define TPGS_MODE_UNINITIALIZED		 -1
#define TPGS_MODE_NONE			0x0
#define TPGS_MODE_IMPLICIT		0x1
#define TPGS_MODE_EXPLICIT		0x2

#define ALUA_INQUIRY_SIZE		36
#define ALUA_FAILOVER_TIMEOUT		60
#define ALUA_FAILOVER_RETRIES		5
#define ALUA_RTPG_DELAY_MSECS		5

/* flags passed from user level */
#define ALUA_OPTIMIZE_STPG		0x01
#define ALUA_RTPG_EXT_HDR_UNSUPP	0x02
/* State machine flags */
#define ALUA_PG_RUN_RTPG		0x10
#define ALUA_PG_RUN_STPG		0x20
#define ALUA_PG_RUNNING			0x40


static LIST_HEAD(port_group_list);
static DEFINE_SPINLOCK(port_group_lock);
static struct workqueue_struct *kmpath_aluad;

struct alua_port_group {
	struct kref		kref;
	struct list_head	node;
	unsigned char		target_id[256];
	unsigned char		target_id_str[256];
	int			target_id_size;
	int			group_id;
	int			lun;
	int			lugrp;
	int			tpgs;
	int			state;
	int			pref;
	unsigned		flags; /* used for optimizing STPG */
	unsigned char		inq[ALUA_INQUIRY_SIZE];
	unsigned char		*buff;
	int			bufflen;
	unsigned char		transition_tmo;
	unsigned long		expiry;
	unsigned long		interval;
	struct delayed_work	rtpg_work;
	spinlock_t		rtpg_lock;
	struct list_head	rtpg_list;
	struct scsi_device	*rtpg_sdev;
};

struct alua_dh_data {
	struct alua_port_group	*pg;
	spinlock_t		pg_lock;
	int			rel_port;
	int			tpgs;
	int			error;
	struct completion       init_complete;
};

struct alua_queue_data {
	struct list_head	entry;
	activate_complete	callback_fn;
	void			*callback_data;
};

#define ALUA_POLICY_SWITCH_CURRENT	0
#define ALUA_POLICY_SWITCH_ALL		1

static char print_alua_state(int);
static int alua_check_sense(struct scsi_device *, struct scsi_sense_hdr *);
static void alua_rtpg_work(struct work_struct *work);
static void alua_check(struct scsi_device *sdev, bool force);

static inline struct alua_dh_data *get_alua_data(struct scsi_device *sdev)
{
	struct scsi_dh_data *scsi_dh_data = sdev->scsi_dh_data;

	return ((struct alua_dh_data *) scsi_dh_data->buf);
}

static int realloc_buffer(struct alua_port_group *pg, unsigned len)
{
	if (pg->buff && pg->buff != pg->inq)
		kfree(pg->buff);

	pg->buff = kmalloc(len, GFP_NOIO);
	if (!pg->buff) {
		pg->buff = pg->inq;
		pg->bufflen = ALUA_INQUIRY_SIZE;
		return 1;
	}
	pg->bufflen = len;
	return 0;
}

static struct request *get_alua_req(struct scsi_device *sdev,
				    void *buffer, unsigned buflen, int rw)
{
	struct request *rq;
	struct request_queue *q = sdev->request_queue;

	rq = blk_get_request(q, rw, GFP_NOIO);

	if (!rq) {
		sdev_printk(KERN_INFO, sdev,
			    "%s: blk_get_request failed\n", __func__);
		return NULL;
	}

	if (buflen && blk_rq_map_kern(q, rq, buffer, buflen, GFP_NOIO)) {
		blk_put_request(rq);
		sdev_printk(KERN_INFO, sdev,
			    "%s: blk_rq_map_kern failed\n", __func__);
		return NULL;
	}

	rq->cmd_type = REQ_TYPE_BLOCK_PC;
	rq->cmd_flags |= REQ_FAILFAST_DEV | REQ_FAILFAST_TRANSPORT |
			 REQ_FAILFAST_DRIVER;
	rq->retries = ALUA_FAILOVER_RETRIES;
	rq->timeout = ALUA_FAILOVER_TIMEOUT * HZ;

	return rq;
}

static void release_port_group(struct kref *kref)
{
	struct alua_port_group *pg;

	pg = container_of(kref, struct alua_port_group, kref);
	printk(KERN_WARNING "alua: release port group %d\n", pg->group_id);
	spin_lock(&port_group_lock);
	list_del(&pg->node);
	spin_unlock(&port_group_lock);
	if (pg->buff && pg->inq != pg->buff)
		kfree(pg->buff);
	kfree(pg);
}

/*
 * submit_rtpg - Issue a REPORT TARGET GROUP STATES command
 * @sdev: sdev the command should be sent to
 */
static unsigned submit_rtpg(struct scsi_device *sdev, unsigned char *buff,
			    int bufflen, unsigned char *sense, int flags)
{
	struct request *rq;
	int err;

	rq = get_alua_req(sdev, buff, bufflen, READ);
	if (!rq) {
		err = DRIVER_BUSY << 24;
		goto done;
	}

	/* Prepare the command. */
	rq->cmd[0] = MAINTENANCE_IN;
	if (!(flags & ALUA_RTPG_EXT_HDR_UNSUPP))
		rq->cmd[1] = MI_REPORT_TARGET_PGS | MI_EXT_HDR_PARAM_FMT;
	else
		rq->cmd[1] = MI_REPORT_TARGET_PGS;
	rq->cmd[6] = (bufflen >> 24) & 0xff;
	rq->cmd[7] = (bufflen >> 16) & 0xff;
	rq->cmd[8] = (bufflen >>  8) & 0xff;
	rq->cmd[9] = bufflen & 0xff;
	rq->cmd_len = COMMAND_SIZE(MAINTENANCE_IN);

	rq->sense = sense;
	memset(rq->sense, 0, SCSI_SENSE_BUFFERSIZE);
	rq->sense_len = 0;

	err = blk_execute_rq(rq->q, NULL, rq, 1);
	if (err < 0) {
		if (!rq->errors)
			err = DID_ERROR << 16;
		else
			err = rq->errors;
		if (rq->sense_len)
			err |= (DRIVER_SENSE << 24);
	}
	blk_put_request(rq);
done:
	return err;
}

/*
 * submit_stpg - Issue a SET TARGET GROUP STATES command
 *
 * Currently we're only setting the current target port group state
 * to 'active/optimized' and let the array firmware figure out
 * the states of the remaining groups.
 */
static unsigned submit_stpg(struct scsi_device *sdev, int group_id,
			    unsigned char *sense)
{
	struct request *rq;
	unsigned char stpg_data[8];
	int stpg_len = 8;
	int err;

	/* Prepare the data buffer */
	memset(stpg_data, 0, stpg_len);
	stpg_data[4] = TPGS_STATE_OPTIMIZED & 0x0f;
	stpg_data[6] = (group_id >> 8) & 0xff;
	stpg_data[7] = group_id & 0xff;

	rq = get_alua_req(sdev, stpg_data, stpg_len, WRITE);
	if (!rq)
		return SCSI_DH_RES_TEMP_UNAVAIL;

	/* Prepare the command. */
	rq->cmd[0] = MAINTENANCE_OUT;
	rq->cmd[1] = MO_SET_TARGET_PGS;
	rq->cmd[6] = (stpg_len >> 24) & 0xff;
	rq->cmd[7] = (stpg_len >> 16) & 0xff;
	rq->cmd[8] = (stpg_len >>  8) & 0xff;
	rq->cmd[9] = stpg_len & 0xff;
	rq->cmd_len = COMMAND_SIZE(MAINTENANCE_OUT);

	rq->sense = sense;
	memset(rq->sense, 0, SCSI_SENSE_BUFFERSIZE);
	rq->sense_len = 0;

	err = blk_execute_rq(rq->q, NULL, rq, 1);
	if (err < 0) {
		if (!rq->errors)
			err = DID_ERROR << 16;
		else
			err = rq->errors;
		if (rq->sense_len)
			err |= (DRIVER_SENSE << 24);
	}
	blk_put_request(rq);

	return err;
}

/*
 * alua_check_tpgs - Evaluate TPGS setting
 * @sdev: device to be checked
 *
 * Examine the TPGS setting of the sdev to find out if ALUA
 * is supported.
 */
static int alua_check_tpgs(struct scsi_device *sdev, struct alua_dh_data *h)
{
	int err = SCSI_DH_OK;

	if (scsi_is_wlun(sdev->lun)) {
		h->tpgs = TPGS_MODE_NONE;
		sdev_printk(KERN_INFO, sdev,
			    "%s: disable for WLUN\n",
			    ALUA_DH_NAME);
		return SCSI_DH_DEV_UNSUPP;
	}
	if (sdev->type != TYPE_DISK &&
	    sdev->type != TYPE_RBC &&
	    sdev->type != TYPE_OSD) {
		h->tpgs = TPGS_MODE_NONE;
		sdev_printk(KERN_INFO, sdev,
			    "%s: disable for non-disk devices\n",
			    ALUA_DH_NAME);
		return SCSI_DH_DEV_UNSUPP;
	}

	h->tpgs = scsi_device_tpgs(sdev);
	switch (h->tpgs) {
	case TPGS_MODE_EXPLICIT|TPGS_MODE_IMPLICIT:
		sdev_printk(KERN_INFO, sdev,
			    "%s: supports implicit and explicit TPGS\n",
			    ALUA_DH_NAME);
		break;
	case TPGS_MODE_EXPLICIT:
		sdev_printk(KERN_INFO, sdev, "%s: supports explicit TPGS\n",
			    ALUA_DH_NAME);
		break;
	case TPGS_MODE_IMPLICIT:
		sdev_printk(KERN_INFO, sdev, "%s: supports implicit TPGS\n",
			    ALUA_DH_NAME);
		break;
	case 0:
		sdev_printk(KERN_INFO, sdev, "%s: not supported\n",
			    ALUA_DH_NAME);
		err = SCSI_DH_DEV_UNSUPP;
		break;
	default:
		sdev_printk(KERN_INFO, sdev,
			    "%s: unsupported TPGS setting %d\n",
			    ALUA_DH_NAME, h->tpgs);
		h->tpgs = TPGS_MODE_NONE;
		err = SCSI_DH_DEV_UNSUPP;
		break;
	}

	return err;
}

/*
 * alua_check_vpd - Evaluate INQUIRY vpd page 0x83
 * @sdev: device to be checked
 *
 * Extract the relative target port and the target port group
 * descriptor from the list of identificators.
 */
static int alua_check_vpd(struct scsi_device *sdev, struct alua_dh_data *h)
{
	char target_id_str[256], *target_id = NULL;
	int target_id_size;
	int group_id = -1, lun = sdev->lun, lugrp = 0;
	unsigned char *d;
	struct alua_port_group *tmp_pg, *pg = NULL;

	if (!sdev->vpd_pg83)
		return SCSI_DH_DEV_UNSUPP;

	/*
	 * Look for the correct descriptor.
	 */
	memset(target_id_str, 0, 256);
	target_id_size = 0;
	d = sdev->vpd_pg83 + 4;
	while (d < sdev->vpd_pg83 + sdev->vpd_pg83_len) {
		switch (d[1] & 0xf) {
		case 0x2:
			/* EUI-64 */
			if ((d[1] & 0x30) == 0x20) {
				target_id_size = d[3];
				target_id = d + 4;
				switch (target_id_size) {
				case 8:
					sprintf(target_id_str,
						"eui.%8phN", d + 4);
					break;
				case 12:
					sprintf(target_id_str,
						"eui.%12phN", d + 4);
					break;
				case 16:
					sprintf(target_id_str,
						"eui.%16phN", d + 4);
					break;
				default:
					target_id_size = 0;
					break;
				}
			}
			break;
		case 0x3:
			/* NAA */
			if ((d[1] & 0x30) == 0x20) {
				target_id_size = d[3];
				target_id = d + 4;
				switch (target_id_size) {
				case 8:
					sprintf(target_id_str,
						"naa.%8phN", d + 4);
					break;
				case 16:
					sprintf(target_id_str,
						"naa.%16phN", d + 4);
					break;
				default:
					target_id_size = 0;
					break;
				}
			}
		case 0x4:
			/* Relative target port */
			h->rel_port = (d[6] << 8) + d[7];
			break;
		case 0x5:
			/* Target port group */
			group_id = (d[6] << 8) + d[7];
			break;
		case 0x6:
			/* Logical unit group */
			lugrp = (d[2] << 8) + d[3];
			/* -1 indicates valid LUN group */
			lun = -1;
			break;
		case 0x8:
			/* SCSI name string */
			if ((d[1] & 0x30) == 0x20) {
				/* SCSI name */
				target_id_size = d[3];
				target_id = d + 4;
				strncpy(target_id_str, d + 4, 256);
				if (target_id_size > 255)
					target_id_size = 255;
				target_id_str[target_id_size] = '\0';
			}
			break;
		default:
			break;
		}
		d += d[3] + 4;
	}

	if (group_id == -1) {
		/*
		 * Internal error; TPGS supported but required
		 * VPD identification descriptors not present.
		 * Disable ALUA support
		 */
		sdev_printk(KERN_INFO, sdev,
			    "%s: No target port descriptors found\n",
			    ALUA_DH_NAME);
		h->tpgs = TPGS_MODE_NONE;
		return SCSI_DH_DEV_UNSUPP;
	}
	if (!target_id_size) {
		/* Check for EMC Clariion extended inquiry */
		if (!strncmp(sdev->vendor, "DGC     ", 8) &&
		    sdev->inquiry_len > 160) {
			target_id_size = sdev->inquiry[160];
			target_id = sdev->inquiry + 161;
			strcpy(target_id_str, "emc.");
			memcpy(target_id_str + 4, target_id, target_id_size);
		}
		/* Check for HP EVA extended inquiry */
		if (!strncmp(sdev->vendor, "HP      ", 8) &&
		    !strncmp(sdev->model, "HSV", 3) &&
		    sdev->inquiry_len > 170) {
			target_id_size = 16;
			target_id = sdev->inquiry + 154;
			strcpy(target_id_str, "naa.");
			memcpy(target_id_str + 4, target_id, target_id_size);
		}
	}

	spin_lock(&port_group_lock);
	pg = NULL;
	if (target_id_size) {
		sdev_printk(KERN_INFO, sdev,
			    "%s: target %s port group %02x "
			    "rel port %02x\n", ALUA_DH_NAME,
			    target_id_str, group_id, h->rel_port);
		list_for_each_entry(tmp_pg, &port_group_list, node) {
			if (tmp_pg->group_id != group_id)
				continue;
			if (tmp_pg->target_id_size != target_id_size)
				continue;
			if (lun != -1) {
				if (tmp_pg->lun != lun)
					continue;
			} else {
				if (tmp_pg->lugrp != lugrp)
					continue;
			}
			if (memcmp(tmp_pg->target_id, target_id,
				   target_id_size))
				continue;
			pg = tmp_pg;
			break;
		}
	} else {
		sdev_printk(KERN_INFO, sdev,
			    "%s: port group %02x rel port %02x\n",
			    ALUA_DH_NAME, group_id, h->rel_port);
	}
	if (pg) {
		kref_get(&pg->kref);
		spin_unlock(&port_group_lock);
		spin_lock(&h->pg_lock);
		rcu_assign_pointer(h->pg, pg);
		spin_unlock(&h->pg_lock);
		synchronize_rcu();
		return SCSI_DH_OK;
	}
	pg = kzalloc(sizeof(struct alua_port_group), GFP_ATOMIC);
	if (!pg) {
		sdev_printk(KERN_WARNING, sdev,
			    "%s: kzalloc port group failed\n",
			    ALUA_DH_NAME);
		/* Temporary failure, bypass */
		spin_unlock(&port_group_lock);
		return SCSI_DH_DEV_TEMP_BUSY;
	}
	if (target_id_size) {
		memcpy(pg->target_id, target_id, target_id_size);
		strncpy(pg->target_id_str, target_id_str, 256);
	} else {
		memset(pg->target_id, 0, 256);
		pg->target_id_str[0] = '\0';
	}
	pg->target_id_size = target_id_size;
	pg->group_id = group_id;
	pg->lun = lun;
	pg->lugrp = lugrp;
	pg->buff = pg->inq;
	pg->bufflen = ALUA_INQUIRY_SIZE;
	pg->tpgs = h->tpgs;
	pg->state = TPGS_STATE_OPTIMIZED;
	kref_init(&pg->kref);
	INIT_DELAYED_WORK(&pg->rtpg_work, alua_rtpg_work);
	INIT_LIST_HEAD(&pg->rtpg_list);
	spin_lock_init(&pg->rtpg_lock);
	list_add(&pg->node, &port_group_list);
	kref_get(&pg->kref);
	spin_unlock(&port_group_lock);
	spin_lock(&h->pg_lock);
	rcu_assign_pointer(h->pg, pg);
	spin_unlock(&h->pg_lock);
	kref_put(&pg->kref, release_port_group);
	synchronize_rcu();
	return SCSI_DH_OK;
}

static char print_alua_state(int state)
{
	switch (state) {
	case TPGS_STATE_OPTIMIZED:
		return 'A';
	case TPGS_STATE_NONOPTIMIZED:
		return 'N';
	case TPGS_STATE_STANDBY:
		return 'S';
	case TPGS_STATE_UNAVAILABLE:
		return 'U';
	case TPGS_STATE_LBA_DEPENDENT:
		return 'L';
	case TPGS_STATE_OFFLINE:
		return 'O';
	case TPGS_STATE_TRANSITIONING:
		return 'T';
	default:
		return 'X';
	}
}

static int alua_check_sense(struct scsi_device *sdev,
			     struct scsi_sense_hdr *sense_hdr)
{
	switch (sense_hdr->sense_key) {
	case NOT_READY:
		if (sense_hdr->asc == 0x04 && sense_hdr->ascq == 0x0a) {
			/*
			 * LUN Not Accessible - ALUA state transition
			 * Kickoff worker to update internal state.
			 */
			alua_check(sdev, false);
			return NEEDS_RETRY;
		}
		break;
	case UNIT_ATTENTION:
		if (sense_hdr->asc == 0x29 && sense_hdr->ascq == 0x00) {
			/*
			 * Power On, Reset, or Bus Device Reset.
			 * Might have obscured a state transition,
			 * so schedule a recheck.
			 */
			alua_check(sdev, true);
			return ADD_TO_MLQUEUE;
		}
		if (sense_hdr->asc == 0x2a && sense_hdr->ascq == 0x06) {
			/*
			 * ALUA state changed
			 */
			alua_check(sdev, true);
			return ADD_TO_MLQUEUE;
		}
		if (sense_hdr->asc == 0x2a && sense_hdr->ascq == 0x07) {
			/*
			 * Implicit ALUA state transition failed
			 */
			alua_check(sdev, true);
			return ADD_TO_MLQUEUE;
		}
		break;
	}

	return SCSI_RETURN_NOT_HANDLED;
}

/*
 * alua_rtpg - Evaluate REPORT TARGET GROUP STATES
 * @sdev: the device to be evaluated.
 *
 * Evaluate the Target Port Group State.
 * Returns SCSI_DH_DEV_OFFLINED if the path is
 * found to be unusable.
 */
static int alua_rtpg(struct scsi_device *sdev, struct alua_port_group *pg)
{
	unsigned char sense[SCSI_SENSE_BUFFERSIZE];
	struct scsi_sense_hdr sense_hdr;
	int len, k, off, valid_states = 0;
	unsigned char *ucp;
	unsigned err, retval;
	unsigned int tpg_desc_tbl_off;
	unsigned char orig_transition_tmo;

	if (!pg->expiry) {
		if (!pg->transition_tmo)
			pg->expiry = round_jiffies_up(jiffies + ALUA_FAILOVER_TIMEOUT * HZ);
		else
			pg->expiry = round_jiffies_up(jiffies + pg->transition_tmo * HZ);
	}
 retry:
	retval = submit_rtpg(sdev, pg->buff, pg->bufflen, sense, pg->flags);

	if (retval) {
		if (!(driver_byte(retval) & DRIVER_SENSE) ||
		    !scsi_normalize_sense(sense, SCSI_SENSE_BUFFERSIZE,
					  &sense_hdr)) {
			sdev_printk(KERN_INFO, sdev, "%s: rtpg failed, ",
				    ALUA_DH_NAME);
			scsi_show_result(retval);
			if (driver_byte(retval) == DRIVER_BUSY)
				err = SCSI_DH_DEV_TEMP_BUSY;
			else
				err = SCSI_DH_IO;
			pg->expiry = 0;
			return err;
		}

		/*
		 * submit_rtpg() has failed on existing arrays
		 * when requesting extended header info, and
		 * the array doesn't support extended headers,
		 * even though it shouldn't according to T10.
		 * The retry without rtpg_ext_hdr_req set
		 * handles this.
		 */
		if (!(pg->flags & ALUA_RTPG_EXT_HDR_UNSUPP) &&
		    sense_hdr.sense_key == ILLEGAL_REQUEST &&
		    sense_hdr.asc == 0x24 && sense_hdr.ascq == 0) {
			pg->flags |= ALUA_RTPG_EXT_HDR_UNSUPP;
			goto retry;
		}

		if (sense_hdr.sense_key == UNIT_ATTENTION)
			err = ADD_TO_MLQUEUE;
		if (err == ADD_TO_MLQUEUE &&
		    pg->expiry != 0 && time_before(jiffies, pg->expiry)) {
			sdev_printk(KERN_ERR, sdev, "%s: rtpg retry, ",
				    ALUA_DH_NAME);
			scsi_show_sense_hdr(&sense_hdr);
			sdev_printk(KERN_ERR, sdev, "%s: rtpg retry, ",
				    ALUA_DH_NAME);
			scsi_show_extd_sense(sense_hdr.asc, sense_hdr.ascq);
			return SCSI_DH_RETRY;
		}
		sdev_printk(KERN_INFO, sdev, "%s: rtpg failed, ",
			    ALUA_DH_NAME);
		scsi_show_sense_hdr(&sense_hdr);
		sdev_printk(KERN_INFO, sdev, "%s: rtpg failed, ",
			    ALUA_DH_NAME);
		scsi_show_extd_sense(sense_hdr.asc, sense_hdr.ascq);
		pg->expiry = 0;
		return SCSI_DH_IO;
	}

	len = (pg->buff[0] << 24) + (pg->buff[1] << 16) +
		(pg->buff[2] << 8) + pg->buff[3] + 4;

	if (len > pg->bufflen) {
		/* Resubmit with the correct length */
		if (realloc_buffer(pg, len)) {
			sdev_printk(KERN_WARNING, sdev,
				    "%s: kmalloc buffer failed\n",__func__);
			/* Temporary failure, bypass */
			pg->expiry = 0;
			return SCSI_DH_DEV_TEMP_BUSY;
		}
		goto retry;
	}

	orig_transition_tmo = pg->transition_tmo;
	if ((pg->buff[4] & RTPG_FMT_MASK) == RTPG_FMT_EXT_HDR &&
	    pg->buff[5] != 0)
		pg->transition_tmo = pg->buff[5];
	else
		pg->transition_tmo = ALUA_FAILOVER_TIMEOUT;

	if (orig_transition_tmo != pg->transition_tmo) {
		sdev_printk(KERN_INFO, sdev,
		       "%s: target %s transition timeout set to %d seconds\n",
		       ALUA_DH_NAME, pg->target_id_str, pg->transition_tmo);
		pg->expiry = jiffies + pg->transition_tmo * HZ;
	}

	if ((pg->buff[4] & RTPG_FMT_MASK) == RTPG_FMT_EXT_HDR)
		tpg_desc_tbl_off = 8;
	else
		tpg_desc_tbl_off = 4;

	for (k = tpg_desc_tbl_off, ucp = pg->buff + tpg_desc_tbl_off;
	     k < len;
	     k += off, ucp += off) {

		if (pg->group_id == (ucp[2] << 8) + ucp[3]) {
			pg->state = ucp[0] & 0x0f;
			pg->pref = ucp[0] >> 7;
			valid_states = ucp[1];
		}
		off = 8 + (ucp[7] * 4);
	}

	sdev_printk(KERN_INFO, sdev,
		    "%s: target %s port group %02x state %c %s "
		    "supports %c%c%c%c%c%c%c\n", ALUA_DH_NAME,
		    pg->target_id_str, pg->group_id,
		    print_alua_state(pg->state),
		    pg->pref ? "preferred" : "non-preferred",
		    valid_states&TPGS_SUPPORT_TRANSITION?'T':'t',
		    valid_states&TPGS_SUPPORT_OFFLINE?'O':'o',
		    valid_states&TPGS_SUPPORT_LBA_DEPENDENT?'L':'l',
		    valid_states&TPGS_SUPPORT_UNAVAILABLE?'U':'u',
		    valid_states&TPGS_SUPPORT_STANDBY?'S':'s',
		    valid_states&TPGS_SUPPORT_NONOPTIMIZED?'N':'n',
		    valid_states&TPGS_SUPPORT_OPTIMIZED?'A':'a');

	switch (pg->state) {
	case TPGS_STATE_TRANSITIONING:
		if (time_before(jiffies, pg->expiry)) {
			/* State transition, retry */
			pg->interval = 2;
			err = SCSI_DH_RETRY;
		} else {
			/* Transitioning time exceeded, set port to standby */
			err = SCSI_DH_IO;
			pg->state = TPGS_STATE_STANDBY;
			pg->expiry = 0;
		}
		break;
	case TPGS_STATE_OFFLINE:
		/* Path unusable */
		err = SCSI_DH_DEV_OFFLINED;
		pg->expiry = 0;
		break;
	default:
		/* Useable path if active */
		err = SCSI_DH_OK;
		pg->expiry = 0;
		break;
	}
	return err;
}

/*
 * alua_stpg - Issue a SET TARGET GROUP STATES command
 *
 * Issue a SET TARGET GROUP STATES command and evaluate the
 * response. Returns SCSI_DH_RETRY per default to trigger
 * a re-evaluation of the target group state.
 */
static unsigned alua_stpg(struct scsi_device *sdev, struct alua_port_group *pg)
{
	int retval, err = SCSI_DH_RETRY;
	unsigned char sense[SCSI_SENSE_BUFFERSIZE];
	struct scsi_sense_hdr sense_hdr;

	if (!(pg->tpgs & TPGS_MODE_EXPLICIT)) {
		/* Only implicit ALUA supported */
		return SCSI_DH_OK;
	}
	switch (pg->state) {
	case TPGS_STATE_OPTIMIZED:
		return SCSI_DH_OK;
	case TPGS_STATE_NONOPTIMIZED:
		if ((pg->flags & ALUA_OPTIMIZE_STPG) &&
		    (!pg->pref) &&
		    (pg->tpgs & TPGS_MODE_IMPLICIT))
			return SCSI_DH_OK;
		break;
	case TPGS_STATE_STANDBY:
	case TPGS_STATE_UNAVAILABLE:
		break;
	case TPGS_STATE_OFFLINE:
		return SCSI_DH_IO;
		break;
	case TPGS_STATE_TRANSITIONING:
		return SCSI_DH_RETRY;
	default:
		sdev_printk(KERN_INFO, sdev,
			    "%s: stpg failed, unhandled TPGS state %d",
			    ALUA_DH_NAME, pg->state);
		return SCSI_DH_NOSYS;
	}
	retval = submit_stpg(sdev, pg->group_id, sense);

	if (retval) {
		if (!(driver_byte(retval) & DRIVER_SENSE) ||
		    !scsi_normalize_sense(sense, SCSI_SENSE_BUFFERSIZE,
					  &sense_hdr)) {
			sdev_printk(KERN_INFO, sdev, "%s: stpg failed, ",
				    ALUA_DH_NAME);
			scsi_show_result(retval);
			/* Retry RTPG */
			return err;
		}
		sdev_printk(KERN_INFO, sdev, "%s: stpg failed, ",
			    ALUA_DH_NAME);
		scsi_show_sense_hdr(&sense_hdr);
		sdev_printk(KERN_INFO, sdev, "%s: stpg failed, ",
			    ALUA_DH_NAME);
		scsi_show_extd_sense(sense_hdr.asc, sense_hdr.ascq);
		err = SCSI_DH_RETRY;
	}
	return err;
}

static void alua_rtpg_work(struct work_struct *work)
{
	struct alua_port_group *pg =
		container_of(work, struct alua_port_group, rtpg_work.work);
	struct scsi_device *sdev;
	LIST_HEAD(qdata_list);
	int err = SCSI_DH_OK;
	struct alua_queue_data *qdata, *tmp;
	unsigned long flags;

	spin_lock_irqsave(&pg->rtpg_lock, flags);
	sdev = pg->rtpg_sdev;
	if (!sdev) {
		WARN_ON(pg->flags & ALUA_PG_RUN_RTPG ||
			pg->flags & ALUA_PG_RUN_STPG);
		spin_unlock_irqrestore(&pg->rtpg_lock, flags);
		return;
	}
	pg->flags |= ALUA_PG_RUNNING;
	if (pg->flags & ALUA_PG_RUN_RTPG) {
		spin_unlock_irqrestore(&pg->rtpg_lock, flags);
		err = alua_rtpg(sdev, pg);
		spin_lock_irqsave(&pg->rtpg_lock, flags);
		if (err == SCSI_DH_RETRY) {
			pg->flags &= ~ALUA_PG_RUNNING;
			spin_unlock_irqrestore(&pg->rtpg_lock, flags);
			queue_delayed_work(kmpath_aluad, &pg->rtpg_work,
					   pg->interval * HZ);
			return;
		}
		pg->flags &= ~ALUA_PG_RUN_RTPG;
		if (err != SCSI_DH_OK)
			pg->flags &= ~ALUA_PG_RUN_STPG;
	}
	if (pg->flags & ALUA_PG_RUN_STPG) {
		spin_unlock_irqrestore(&pg->rtpg_lock, flags);
		err = alua_stpg(sdev, pg);
		spin_lock_irqsave(&pg->rtpg_lock, flags);
		pg->flags &= ~ALUA_PG_RUN_STPG;
		if (err == SCSI_DH_RETRY) {
			pg->flags |= ALUA_PG_RUN_RTPG;
			pg->interval = 0;
			pg->flags &= ~ALUA_PG_RUNNING;
			spin_unlock_irqrestore(&pg->rtpg_lock, flags);
			queue_delayed_work(kmpath_aluad, &pg->rtpg_work,
				msecs_to_jiffies(ALUA_RTPG_DELAY_MSECS));
			return;
		}
	}

	list_splice_init(&pg->rtpg_list, &qdata_list);
	pg->rtpg_sdev = NULL;
	spin_unlock_irqrestore(&pg->rtpg_lock, flags);

	list_for_each_entry_safe(qdata, tmp, &qdata_list, entry) {
		list_del(&qdata->entry);
		if (qdata->callback_fn)
			qdata->callback_fn(qdata->callback_data, err);
		kfree(qdata);
	}
	spin_lock_irqsave(&pg->rtpg_lock, flags);
	pg->flags &= ~ALUA_PG_RUNNING;
	spin_unlock_irqrestore(&pg->rtpg_lock, flags);
	kref_put(&pg->kref, release_port_group);
	scsi_device_put(sdev);
}

static void alua_rtpg_queue(struct alua_port_group *pg,
			    struct scsi_device *sdev,
			    struct alua_queue_data *qdata, bool force)
{
	int start_queue = 0;
	unsigned long flags;

	if (!pg)
		return;

	kref_get(&pg->kref);
	spin_lock_irqsave(&pg->rtpg_lock, flags);
	if (qdata) {
		list_add_tail(&qdata->entry, &pg->rtpg_list);
		pg->flags |= ALUA_PG_RUN_STPG;
	}
	if (pg->rtpg_sdev == NULL) {
		pg->interval = 0;
		pg->flags |= ALUA_PG_RUN_RTPG;
		kref_get(&pg->kref);
		pg->rtpg_sdev = sdev;
		scsi_device_get(sdev);
		start_queue = 1;
	} else if (!(pg->flags & ALUA_PG_RUNNING) && force)
		start_queue = 1;

	spin_unlock_irqrestore(&pg->rtpg_lock, flags);

	if (start_queue)
		mod_delayed_work(kmpath_aluad, &pg->rtpg_work,
				   msecs_to_jiffies(ALUA_RTPG_DELAY_MSECS));

	kref_put(&pg->kref, release_port_group);
}

/*
 * alua_initialize - Initialize ALUA state
 * @sdev: the device to be initialized
 *
 * For the prep_fn to work correctly we have
 * to initialize the ALUA state for the device.
 */
static int alua_initialize(struct scsi_device *sdev, struct alua_dh_data *h)
{
	struct alua_port_group *pg = NULL;

	h->error = alua_check_tpgs(sdev, h);
	if (h->error == SCSI_DH_OK) {
		h->error = alua_check_vpd(sdev, h);
		rcu_read_lock();
		pg = rcu_dereference(h->pg);
		if (!pg) {
			rcu_read_unlock();
			h->tpgs = TPGS_MODE_NONE;
			if (h->error == SCSI_DH_OK)
				h->error = SCSI_DH_IO;
		} else {
			kref_get(&pg->kref);
			rcu_read_unlock();
		}
	}
	complete(&h->init_complete);
	if (pg) {
		pg->expiry = 0;
		alua_rtpg_queue(pg, sdev, NULL, true);
		kref_put(&pg->kref, release_port_group);
	}
	return h->error;
}

/*
 * alua_set_params - set/unset the optimize flag
 * @sdev: device on the path to be activated
 * params - parameters in the following format
 *      "no_of_params\0param1\0param2\0param3\0...\0"
 * For example, to set the flag pass the following parameters
 * from multipath.conf
 *     hardware_handler        "2 alua 1"
 */
static int alua_set_params(struct scsi_device *sdev, const char *params)
{
	struct alua_dh_data *h = get_alua_data(sdev);
	struct alua_port_group *pg = NULL;
	unsigned int optimize = 0, argc;
	const char *p = params;
	int result = SCSI_DH_OK;
	unsigned long flags;

	if (!h)
		return -ENXIO;

	if ((sscanf(params, "%u", &argc) != 1) || (argc != 1))
		return -EINVAL;

	while (*p++)
		;
	if ((sscanf(p, "%u", &optimize) != 1) || (optimize > 1))
		return -EINVAL;

	rcu_read_lock();
	pg = rcu_dereference(h->pg);
	if (!pg) {
		rcu_read_unlock();
		return -ENXIO;
	}
	rcu_read_unlock();

	spin_lock_irqsave(&pg->rtpg_lock, flags);
	if (optimize)
		pg->flags |= ALUA_OPTIMIZE_STPG;
	else
		pg->flags |= ~ALUA_OPTIMIZE_STPG;

	spin_unlock_irqrestore(&pg->rtpg_lock, flags);
	return result;
}

static uint optimize_stpg;
module_param(optimize_stpg, uint, S_IRUGO|S_IWUSR);
MODULE_PARM_DESC(optimize_stpg, "Allow use of a non-optimized path, rather than sending a STPG, when implicit TPGS is supported (0=No,1=Yes). Default is 0.");

/*
 * alua_activate - activate a path
 * @sdev: device on the path to be activated
 *
 * We're currently switching the port group to be activated only and
 * let the array figure out the rest.
 * There may be other arrays which require us to switch all port groups
 * based on a certain policy. But until we actually encounter them it
 * should be okay.
 */
static int alua_activate(struct scsi_device *sdev,
			activate_complete fn, void *data)
{
	struct alua_dh_data *h = get_alua_data(sdev);
	struct alua_queue_data *qdata;
	struct alua_port_group *pg;
	unsigned long flags;

	if (!h) {
		if (fn)
			fn(data, SCSI_DH_NOSYS);
		return 0;
	}

	qdata = kzalloc(sizeof(*qdata), GFP_KERNEL);
	if (!qdata) {
		if (fn)
			fn(data, SCSI_DH_RETRY);
		return 0;
	}

	qdata->callback_fn = fn;
	qdata->callback_data = data;

	rcu_read_lock();
	pg = rcu_dereference(h->pg);
	if (!pg) {
		rcu_read_unlock();
		wait_for_completion(&h->init_complete);
		rcu_read_lock();
		pg = rcu_dereference(h->pg);
		if (!pg) {
			rcu_read_unlock();
			kfree(qdata);
			if (fn)
				fn(data, h->error);
			return 0;
		}
	}
	kref_get(&pg->kref);
	rcu_read_unlock();

	if (optimize_stpg) {
		spin_lock_irqsave(&pg->rtpg_lock, flags);
		pg->flags |= ALUA_OPTIMIZE_STPG;
		spin_unlock_irqrestore(&pg->rtpg_lock, flags);
	}

	alua_rtpg_queue(pg, sdev, qdata, true);
	kref_put(&pg->kref, release_port_group);
	return 0;
}

/*
 * alua_check - check path status
 * @sdev: device on the path to be checked
 *
 * Check the device status
 */
static void alua_check(struct scsi_device *sdev, bool force)
{
	struct alua_dh_data *h = get_alua_data(sdev);
	struct alua_port_group *pg;

	if (!h)
		return;

	rcu_read_lock();
	pg = rcu_dereference(h->pg);
	if (pg) {
		kref_get(&pg->kref);
		rcu_read_unlock();
		alua_rtpg_queue(pg, sdev, NULL, force);
		kref_put(&pg->kref, release_port_group);
	} else
		rcu_read_unlock();
}

/*
 * alua_prep_fn - request callback
 *
 * Fail I/O to all paths not in state
 * active/optimized or active/non-optimized.
 */
static int alua_prep_fn(struct scsi_device *sdev, struct request *req)
{
	struct alua_dh_data *h = get_alua_data(sdev);
	struct alua_port_group *pg;
	int state = TPGS_STATE_OPTIMIZED;
	int ret = BLKPREP_OK;

	if (!h)
		return ret;

	rcu_read_lock();
	pg = rcu_dereference(h->pg);
	if (pg) {
		state = pg->state;
		/* Defer I/O while rtpg_work is active */
		if (pg->rtpg_sdev)
			state = TPGS_STATE_TRANSITIONING;
	}
	rcu_read_unlock();
	if (state == TPGS_STATE_TRANSITIONING)
		ret = BLKPREP_DEFER;
	else if (state != TPGS_STATE_OPTIMIZED &&
		 state != TPGS_STATE_NONOPTIMIZED &&
		 state != TPGS_STATE_LBA_DEPENDENT) {
		ret = BLKPREP_KILL;
		req->cmd_flags |= REQ_QUIET;
	}
	return ret;

}

static bool alua_match(struct scsi_device *sdev)
{
	return (scsi_device_tpgs(sdev) != 0);
}

static int alua_bus_attach(struct scsi_device *sdev);
static void alua_bus_detach(struct scsi_device *sdev);

static struct scsi_device_handler alua_dh = {
	.name = ALUA_DH_NAME,
	.module = THIS_MODULE,
	.attach = alua_bus_attach,
	.detach = alua_bus_detach,
	.prep_fn = alua_prep_fn,
	.check_sense = alua_check_sense,
	.activate = alua_activate,
	.set_params = alua_set_params,
	.match = alua_match,
};

/*
 * alua_bus_attach - Attach device handler
 * @sdev: device to be attached to
 */
static int alua_bus_attach(struct scsi_device *sdev)
{
	struct scsi_dh_data *scsi_dh_data;
	struct alua_dh_data *h;
	unsigned long flags;
	int err = SCSI_DH_OK;

	scsi_dh_data = kzalloc(sizeof(*scsi_dh_data)
			       + sizeof(*h) , GFP_KERNEL);
	if (!scsi_dh_data) {
		sdev_printk(KERN_ERR, sdev, "%s: Attach failed\n",
			    ALUA_DH_NAME);
		return -ENOMEM;
	}

	scsi_dh_data->scsi_dh = &alua_dh;
	h = (struct alua_dh_data *) scsi_dh_data->buf;
	spin_lock_init(&h->pg_lock);
	h->tpgs = TPGS_MODE_UNINITIALIZED;
	rcu_assign_pointer(h->pg, NULL);
	h->rel_port = -1;
	h->error = SCSI_DH_OK;
	init_completion(&h->init_complete);
	err = alua_initialize(sdev, h);
	if ((err != SCSI_DH_OK) && (err != SCSI_DH_DEV_OFFLINED))
		goto failed;

	if (!try_module_get(THIS_MODULE))
		goto failed;

	spin_lock_irqsave(sdev->request_queue->queue_lock, flags);
	sdev->scsi_dh_data = scsi_dh_data;
	spin_unlock_irqrestore(sdev->request_queue->queue_lock, flags);
	sdev_printk(KERN_NOTICE, sdev, "%s: Attached\n", ALUA_DH_NAME);

	return 0;

failed:
	kfree(scsi_dh_data);
	sdev_printk(KERN_ERR, sdev, "%s: not attached\n", ALUA_DH_NAME);
	return -EINVAL;
}

/*
 * alua_bus_detach - Detach device handler
 * @sdev: device to be detached from
 */
static void alua_bus_detach(struct scsi_device *sdev)
{
	struct scsi_dh_data *scsi_dh_data;
	struct alua_dh_data *h;
	struct alua_port_group *pg = NULL;
	unsigned long flags;

	spin_lock_irqsave(sdev->request_queue->queue_lock, flags);
	scsi_dh_data = sdev->scsi_dh_data;
	sdev->scsi_dh_data = NULL;
	h = (struct alua_dh_data *) scsi_dh_data->buf;
	spin_lock(&h->pg_lock);
	pg = h->pg;
	rcu_assign_pointer(h->pg, NULL);
	spin_unlock(&h->pg_lock);
	spin_unlock_irqrestore(sdev->request_queue->queue_lock, flags);
	synchronize_rcu();
	if (pg) {
		if (pg->rtpg_sdev)
			flush_workqueue(kmpath_aluad);
		kref_put(&pg->kref, release_port_group);
	}
	kfree(scsi_dh_data);
	module_put(THIS_MODULE);
	sdev_printk(KERN_NOTICE, sdev, "%s: Detached\n", ALUA_DH_NAME);
}

static int __init alua_init(void)
{
	int r;

	kmpath_aluad = create_singlethread_workqueue("kmpath_aluad");
	if (!kmpath_aluad) {
		printk(KERN_ERR "kmpath_aluad creation failed.\n");
		return -EINVAL;
	}
	r = scsi_register_device_handler(&alua_dh);
	if (r != 0) {
		printk(KERN_ERR "%s: Failed to register scsi device handler",
			ALUA_DH_NAME);
		destroy_workqueue(kmpath_aluad);
	}
	return r;
}

static void __exit alua_exit(void)
{
	scsi_unregister_device_handler(&alua_dh);
	destroy_workqueue(kmpath_aluad);
}

module_init(alua_init);
module_exit(alua_exit);

MODULE_DESCRIPTION("DM Multipath ALUA support");
MODULE_AUTHOR("Hannes Reinecke <hare@suse.de>");
MODULE_LICENSE("GPL");
MODULE_VERSION(ALUA_DH_VER);
