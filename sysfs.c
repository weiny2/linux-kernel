/*
 *
 * This file is provided under a dual BSD/GPLv2 license.  When using or
 * redistributing this file, you may do so under either license.
 *
 * GPL LICENSE SUMMARY
 *
 * Copyright(c) 2015 Intel Corporation.
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
 * BSD LICENSE
 *
 * Copyright(c) 2015 Intel Corporation.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 *  - Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *  - Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the
 *    distribution.
 *  - Neither the name of Intel Corporation nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
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
 *
 */
#include <linux/ctype.h>

#include "hfi.h"
#include "mad.h"
#include "trace.h"

/* start of per-port functions */
/*
 * Get/Set heartbeat enable. OR of 1=enabled, 2=auto
 */
static ssize_t show_hrtbt_enb(struct hfi1_pportdata *ppd, char *buf)
{
	int ret;

	ret = hfi1_get_ib_cfg(ppd, HFI1_IB_CFG_HRTBT);
	ret = scnprintf(buf, PAGE_SIZE, "%d\n", ret);
	return ret;
}

static ssize_t store_hrtbt_enb(struct hfi1_pportdata *ppd, const char *buf,
			       size_t count)
{
	struct hfi_devdata *dd = ppd->dd;
	int ret;
	u16 val;

	ret = kstrtou16(buf, 0, &val);
	if (ret) {
		dd_dev_err(dd, "attempt to set invalid Heartbeat enable\n");
		return ret;
	}

	/*
	 * Set the "intentional" heartbeat enable per either of
	 * "Enable" and "Auto", as these are normally set together.
	 * This bit is consulted when leaving loopback mode,
	 * because entering loopback mode overrides it and automatically
	 * disables heartbeat.
	 */
	ret = hfi1_set_ib_cfg(ppd, HFI1_IB_CFG_HRTBT, val);
	return ret < 0 ? ret : count;
}

static ssize_t store_led_override(struct hfi1_pportdata *ppd, const char *buf,
				  size_t count)
{
	struct hfi_devdata *dd = ppd->dd;
	int ret;
	u16 val;

	ret = kstrtou16(buf, 0, &val);
	if (ret) {
		dd_dev_err(dd, "attempt to set invalid LED override\n");
		return ret;
	}

	hfi1_set_led_override(ppd, val);
	return count;
}

static ssize_t show_status(struct hfi1_pportdata *ppd, char *buf)
{
	ssize_t ret;

	if (!ppd->statusp)
		ret = -EINVAL;
	else
		ret = scnprintf(buf, PAGE_SIZE, "0x%llx\n",
				(unsigned long long) *(ppd->statusp));
	return ret;
}

/*
 * For userland compatibility, these offsets must remain fixed.
 * They are strings for HFI_STATUS_*
 */
static const char * const hfi1_status_str[] = {
	"Initted",
	"",
	"",
	"",
	"",
	"Present",
	"IB_link_up",
	"IB_configured",
	"",
	"Fatal_Hardware_Error",
	NULL,
};

static ssize_t show_status_str(struct hfi1_pportdata *ppd, char *buf)
{
	int i, any;
	u64 s;
	ssize_t ret;

	if (!ppd->statusp) {
		ret = -EINVAL;
		goto bail;
	}

	s = *(ppd->statusp);
	*buf = '\0';
	for (any = i = 0; s && hfi1_status_str[i]; i++) {
		if (s & 1) {
			/* if overflow */
			if (any && strlcat(buf, " ", PAGE_SIZE) >= PAGE_SIZE)
				break;
			if (strlcat(buf, hfi1_status_str[i], PAGE_SIZE) >=
					PAGE_SIZE)
				break;
			any = 1;
		}
		s >>= 1;
	}
	if (any)
		strlcat(buf, "\n", PAGE_SIZE);

	ret = strlen(buf);

bail:
	return ret;
}

/* end of per-port functions */

/*
 * Start of per-port file structures and support code
 * Because we are fitting into other infrastructure, we have to supply the
 * full set of kobject/sysfs_ops structures and routines.
 */
#define HFI1_PORT_ATTR(name, mode, show, store) \
	static struct hfi1_port_attr port_attr_##name = \
		__ATTR(name, mode, show, store)

struct hfi1_port_attr {
	struct attribute attr;
	ssize_t (*show)(struct hfi1_pportdata *, char *);
	ssize_t (*store)(struct hfi1_pportdata *, const char *, size_t);
};

HFI1_PORT_ATTR(led_override, S_IWUSR, NULL, store_led_override);
HFI1_PORT_ATTR(hrtbt_enable, S_IWUSR | S_IRUGO, show_hrtbt_enb,
	       store_hrtbt_enb);
HFI1_PORT_ATTR(status, S_IRUGO, show_status, NULL);
HFI1_PORT_ATTR(status_str, S_IRUGO, show_status_str, NULL);

static struct attribute *port_default_attributes[] = {
	&port_attr_led_override.attr,
	&port_attr_hrtbt_enable.attr,
	&port_attr_status.attr,
	&port_attr_status_str.attr,
	NULL
};

/*
 * Start of per-port congestion control structures and support code
 */

/*
 * Congestion control table size followed by table entries
 */
static ssize_t read_cc_table_bin(struct file *filp, struct kobject *kobj,
		struct bin_attribute *bin_attr,
		char *buf, loff_t pos, size_t count)
{
	int ret;
	struct hfi1_pportdata *ppd =
		container_of(kobj, struct hfi1_pportdata, pport_cc_kobj);
	struct cc_state *cc_state;

	ret = ppd->total_cct_entry * sizeof(struct ib_cc_table_entry_shadow)
		 + sizeof(__be16);

	if (pos > ret)
		return -EINVAL;

	if (count > ret - pos)
		count = ret - pos;

	if (!count)
		return count;

	rcu_read_lock();
	cc_state = get_cc_state(ppd);
	if (cc_state == NULL) {
		rcu_read_unlock();
		return -EINVAL;
	}
	memcpy(buf, &cc_state->cct, count);
	rcu_read_unlock();

	return count;
}

static void port_release(struct kobject *kobj)
{
	/* nothing to do since memory is freed by hfi1_free_devdata() */
}

static struct kobj_type port_cc_ktype = {
	.release = port_release,
};

static struct bin_attribute cc_table_bin_attr = {
	.attr = {.name = "cc_table_bin", .mode = 0444},
	.read = read_cc_table_bin,
	.size = PAGE_SIZE,
};

/*
 * Congestion settings: port control, control map and an array of 16
 * entries for the congestion entries - increase, timer, event log
 * trigger threshold and the minimum injection rate delay.
 */
static ssize_t read_cc_setting_bin(struct file *filp, struct kobject *kobj,
		struct bin_attribute *bin_attr,
		char *buf, loff_t pos, size_t count)
{
	int ret;
	struct hfi1_pportdata *ppd =
		container_of(kobj, struct hfi1_pportdata, pport_cc_kobj);
	struct cc_state *cc_state;

	ret = sizeof(struct opa_congestion_setting_attr_shadow);

	if (pos > ret)
		return -EINVAL;
	if (count > ret - pos)
		count = ret - pos;

	if (!count)
		return count;

	rcu_read_lock();
	cc_state = get_cc_state(ppd);
	if (cc_state == NULL) {
		rcu_read_unlock();
		return -EINVAL;
	}
	memcpy(buf, &cc_state->cong_setting, count);
	rcu_read_unlock();

	return count;
}

static struct bin_attribute cc_setting_bin_attr = {
	.attr = {.name = "cc_settings_bin", .mode = 0444},
	.read = read_cc_setting_bin,
	.size = PAGE_SIZE,
};


static ssize_t portattr_show(struct kobject *kobj,
			     struct attribute *attr, char *buf)
{
	struct hfi1_port_attr *pattr =
		container_of(attr, struct hfi1_port_attr, attr);
	struct hfi1_pportdata *ppd =
		container_of(kobj, struct hfi1_pportdata, pport_kobj);

	return pattr->show(ppd, buf);
}

static ssize_t portattr_store(struct kobject *kobj,
			      struct attribute *attr,
			      const char *buf, size_t len)
{
	struct hfi1_port_attr *pattr =
		container_of(attr, struct hfi1_port_attr, attr);
	struct hfi1_pportdata *ppd =
		container_of(kobj, struct hfi1_pportdata, pport_kobj);

	return pattr->store(ppd, buf, len);
}


static const struct sysfs_ops hfi1_port_ops = {
	.show = portattr_show,
	.store = portattr_store,
};

static struct kobj_type hfi1_port_ktype = {
	.release = port_release,
	.sysfs_ops = &hfi1_port_ops,
	.default_attrs = port_default_attributes
};

/* Start sc2vl */
#define HFI_SC2VL_ATTR(N)				    \
	static struct hfi_sc2vl_attr hfi_sc2vl_attr_##N = { \
		.attr = { .name = __stringify(N), .mode = 0444 }, \
		.sc = N \
	}

struct hfi_sc2vl_attr {
	struct attribute attr;
	int sc;
};

HFI_SC2VL_ATTR(0);
HFI_SC2VL_ATTR(1);
HFI_SC2VL_ATTR(2);
HFI_SC2VL_ATTR(3);
HFI_SC2VL_ATTR(4);
HFI_SC2VL_ATTR(5);
HFI_SC2VL_ATTR(6);
HFI_SC2VL_ATTR(7);
HFI_SC2VL_ATTR(8);
HFI_SC2VL_ATTR(9);
HFI_SC2VL_ATTR(10);
HFI_SC2VL_ATTR(11);
HFI_SC2VL_ATTR(12);
HFI_SC2VL_ATTR(13);
HFI_SC2VL_ATTR(14);
HFI_SC2VL_ATTR(15);
HFI_SC2VL_ATTR(16);
HFI_SC2VL_ATTR(17);
HFI_SC2VL_ATTR(18);
HFI_SC2VL_ATTR(19);
HFI_SC2VL_ATTR(20);
HFI_SC2VL_ATTR(21);
HFI_SC2VL_ATTR(22);
HFI_SC2VL_ATTR(23);
HFI_SC2VL_ATTR(24);
HFI_SC2VL_ATTR(25);
HFI_SC2VL_ATTR(26);
HFI_SC2VL_ATTR(27);
HFI_SC2VL_ATTR(28);
HFI_SC2VL_ATTR(29);
HFI_SC2VL_ATTR(30);
HFI_SC2VL_ATTR(31);


static struct attribute *sc2vl_default_attributes[] = {
	&hfi_sc2vl_attr_0.attr,
	&hfi_sc2vl_attr_1.attr,
	&hfi_sc2vl_attr_2.attr,
	&hfi_sc2vl_attr_3.attr,
	&hfi_sc2vl_attr_4.attr,
	&hfi_sc2vl_attr_5.attr,
	&hfi_sc2vl_attr_6.attr,
	&hfi_sc2vl_attr_7.attr,
	&hfi_sc2vl_attr_8.attr,
	&hfi_sc2vl_attr_9.attr,
	&hfi_sc2vl_attr_10.attr,
	&hfi_sc2vl_attr_11.attr,
	&hfi_sc2vl_attr_12.attr,
	&hfi_sc2vl_attr_13.attr,
	&hfi_sc2vl_attr_14.attr,
	&hfi_sc2vl_attr_15.attr,
	&hfi_sc2vl_attr_16.attr,
	&hfi_sc2vl_attr_17.attr,
	&hfi_sc2vl_attr_18.attr,
	&hfi_sc2vl_attr_19.attr,
	&hfi_sc2vl_attr_20.attr,
	&hfi_sc2vl_attr_21.attr,
	&hfi_sc2vl_attr_22.attr,
	&hfi_sc2vl_attr_23.attr,
	&hfi_sc2vl_attr_24.attr,
	&hfi_sc2vl_attr_25.attr,
	&hfi_sc2vl_attr_26.attr,
	&hfi_sc2vl_attr_27.attr,
	&hfi_sc2vl_attr_28.attr,
	&hfi_sc2vl_attr_29.attr,
	&hfi_sc2vl_attr_30.attr,
	&hfi_sc2vl_attr_31.attr,
	NULL
};

static ssize_t sc2vl_attr_show(struct kobject *kobj, struct attribute *attr,
			       char *buf)
{
	struct hfi_sc2vl_attr *sattr =
		container_of(attr, struct hfi_sc2vl_attr, attr);
	struct hfi1_pportdata *ppd =
		container_of(kobj, struct hfi1_pportdata, sc2vl_kobj);
	struct hfi_devdata *dd = ppd->dd;

	return sprintf(buf, "%u\n", *((u8 *)dd->sc2vl + sattr->sc));
}

static const struct sysfs_ops hfi_sc2vl_ops = {
	.show = sc2vl_attr_show,
};

static struct kobj_type hfi_sc2vl_ktype = {
	.release = port_release,
	.sysfs_ops = &hfi_sc2vl_ops,
	.default_attrs = sc2vl_default_attributes
};

/* End sc2vl */

/* Start sl2sc */
#define HFI_SL2SC_ATTR(N)				    \
	static struct hfi_sl2sc_attr hfi_sl2sc_attr_##N = {	  \
		.attr = { .name = __stringify(N), .mode = 0444 }, \
		.sl = N						  \
	}

struct hfi_sl2sc_attr {
	struct attribute attr;
	int sl;
};

HFI_SL2SC_ATTR(0);
HFI_SL2SC_ATTR(1);
HFI_SL2SC_ATTR(2);
HFI_SL2SC_ATTR(3);
HFI_SL2SC_ATTR(4);
HFI_SL2SC_ATTR(5);
HFI_SL2SC_ATTR(6);
HFI_SL2SC_ATTR(7);
HFI_SL2SC_ATTR(8);
HFI_SL2SC_ATTR(9);
HFI_SL2SC_ATTR(10);
HFI_SL2SC_ATTR(11);
HFI_SL2SC_ATTR(12);
HFI_SL2SC_ATTR(13);
HFI_SL2SC_ATTR(14);
HFI_SL2SC_ATTR(15);
HFI_SL2SC_ATTR(16);
HFI_SL2SC_ATTR(17);
HFI_SL2SC_ATTR(18);
HFI_SL2SC_ATTR(19);
HFI_SL2SC_ATTR(20);
HFI_SL2SC_ATTR(21);
HFI_SL2SC_ATTR(22);
HFI_SL2SC_ATTR(23);
HFI_SL2SC_ATTR(24);
HFI_SL2SC_ATTR(25);
HFI_SL2SC_ATTR(26);
HFI_SL2SC_ATTR(27);
HFI_SL2SC_ATTR(28);
HFI_SL2SC_ATTR(29);
HFI_SL2SC_ATTR(30);
HFI_SL2SC_ATTR(31);


static struct attribute *sl2sc_default_attributes[] = {
	&hfi_sl2sc_attr_0.attr,
	&hfi_sl2sc_attr_1.attr,
	&hfi_sl2sc_attr_2.attr,
	&hfi_sl2sc_attr_3.attr,
	&hfi_sl2sc_attr_4.attr,
	&hfi_sl2sc_attr_5.attr,
	&hfi_sl2sc_attr_6.attr,
	&hfi_sl2sc_attr_7.attr,
	&hfi_sl2sc_attr_8.attr,
	&hfi_sl2sc_attr_9.attr,
	&hfi_sl2sc_attr_10.attr,
	&hfi_sl2sc_attr_11.attr,
	&hfi_sl2sc_attr_12.attr,
	&hfi_sl2sc_attr_13.attr,
	&hfi_sl2sc_attr_14.attr,
	&hfi_sl2sc_attr_15.attr,
	&hfi_sl2sc_attr_16.attr,
	&hfi_sl2sc_attr_17.attr,
	&hfi_sl2sc_attr_18.attr,
	&hfi_sl2sc_attr_19.attr,
	&hfi_sl2sc_attr_20.attr,
	&hfi_sl2sc_attr_21.attr,
	&hfi_sl2sc_attr_22.attr,
	&hfi_sl2sc_attr_23.attr,
	&hfi_sl2sc_attr_24.attr,
	&hfi_sl2sc_attr_25.attr,
	&hfi_sl2sc_attr_26.attr,
	&hfi_sl2sc_attr_27.attr,
	&hfi_sl2sc_attr_28.attr,
	&hfi_sl2sc_attr_29.attr,
	&hfi_sl2sc_attr_30.attr,
	&hfi_sl2sc_attr_31.attr,
	NULL
};

static ssize_t sl2sc_attr_show(struct kobject *kobj, struct attribute *attr,
			       char *buf)
{
	struct hfi_sl2sc_attr *sattr =
		container_of(attr, struct hfi_sl2sc_attr, attr);
	struct hfi1_pportdata *ppd =
		container_of(kobj, struct hfi1_pportdata, sl2sc_kobj);
	struct hfi1_ibport *ibp = &ppd->ibport_data;

	return sprintf(buf, "%u\n", ibp->sl_to_sc[sattr->sl]);
}

static const struct sysfs_ops hfi_sl2sc_ops = {
	.show = sl2sc_attr_show,
};

static struct kobj_type hfi_sl2sc_ktype = {
	.release = port_release,
	.sysfs_ops = &hfi_sl2sc_ops,
	.default_attrs = sl2sc_default_attributes
};

/* End sl2sc */

/* Start vl2mtu */

#define HFI_VL2MTU_ATTR(N) \
	static struct hfi_vl2mtu_attr hfi_vl2mtu_attr_##N = { \
		.attr = { .name = __stringify(N), .mode = 0444 }, \
		.vl = N						  \
	}

struct hfi_vl2mtu_attr {
	struct attribute attr;
	int vl;
};

HFI_VL2MTU_ATTR(0);
HFI_VL2MTU_ATTR(1);
HFI_VL2MTU_ATTR(2);
HFI_VL2MTU_ATTR(3);
HFI_VL2MTU_ATTR(4);
HFI_VL2MTU_ATTR(5);
HFI_VL2MTU_ATTR(6);
HFI_VL2MTU_ATTR(7);
HFI_VL2MTU_ATTR(8);
HFI_VL2MTU_ATTR(9);
HFI_VL2MTU_ATTR(10);
HFI_VL2MTU_ATTR(11);
HFI_VL2MTU_ATTR(12);
HFI_VL2MTU_ATTR(13);
HFI_VL2MTU_ATTR(14);
HFI_VL2MTU_ATTR(15);

static struct attribute *vl2mtu_default_attributes[] = {
	&hfi_vl2mtu_attr_0.attr,
	&hfi_vl2mtu_attr_1.attr,
	&hfi_vl2mtu_attr_2.attr,
	&hfi_vl2mtu_attr_3.attr,
	&hfi_vl2mtu_attr_4.attr,
	&hfi_vl2mtu_attr_5.attr,
	&hfi_vl2mtu_attr_6.attr,
	&hfi_vl2mtu_attr_7.attr,
	&hfi_vl2mtu_attr_8.attr,
	&hfi_vl2mtu_attr_9.attr,
	&hfi_vl2mtu_attr_10.attr,
	&hfi_vl2mtu_attr_11.attr,
	&hfi_vl2mtu_attr_12.attr,
	&hfi_vl2mtu_attr_13.attr,
	&hfi_vl2mtu_attr_14.attr,
	&hfi_vl2mtu_attr_15.attr,
	NULL
};

static ssize_t vl2mtu_attr_show(struct kobject *kobj, struct attribute *attr,
				char *buf)
{
	struct hfi_vl2mtu_attr *vlattr =
		container_of(attr, struct hfi_vl2mtu_attr, attr);
	struct hfi1_pportdata *ppd =
		container_of(kobj, struct hfi1_pportdata, vl2mtu_kobj);
	struct hfi_devdata *dd = ppd->dd;

	return sprintf(buf, "%u\n", dd->vld[vlattr->vl].mtu);
}

static const struct sysfs_ops hfi_vl2mtu_ops = {
	.show = vl2mtu_attr_show,
};

static struct kobj_type hfi_vl2mtu_ktype = {
	.release = port_release,
	.sysfs_ops = &hfi_vl2mtu_ops,
	.default_attrs = vl2mtu_default_attributes
};

/* Start diag_counters */
#define HFI1_DIAGC_NORMAL 0x0
#define HFI1_DIAGC_PCPU   0x1

#define HFI1_DIAGC_ATTR(N) \
	static struct hfi1_diagc_attr diagc_attr_##N = { \
		.attr = { .name = __stringify(N), .mode = 0664 }, \
		.counter = offsetof(struct hfi1_ibport, n_##N), \
		.type = HFI1_DIAGC_NORMAL, \
		.vl = CNTR_INVALID_VL \
	}

#define HFI1_DIAGC_ATTR_PCPU(N, V, L) \
	static struct hfi1_diagc_attr diagc_attr_##N = { \
		.attr = { .name = __stringify(N), .mode = 0664 }, \
		.counter = V, \
		.type = HFI1_DIAGC_PCPU, \
		.vl = L \
	}

struct hfi1_diagc_attr {
	struct attribute attr;
	size_t counter;
	int type;
	int vl;
};

HFI1_DIAGC_ATTR(rc_resends);
HFI1_DIAGC_ATTR_PCPU(rc_acks, C_SW_CPU_RC_ACKS, CNTR_INVALID_VL);
HFI1_DIAGC_ATTR_PCPU(rc_qacks, C_SW_CPU_RC_QACKS, CNTR_INVALID_VL);
HFI1_DIAGC_ATTR(rc_delayed_comp);
HFI1_DIAGC_ATTR(seq_naks);
HFI1_DIAGC_ATTR(rdma_seq);
HFI1_DIAGC_ATTR(rnr_naks);
HFI1_DIAGC_ATTR(other_naks);
HFI1_DIAGC_ATTR(rc_timeouts);
HFI1_DIAGC_ATTR(loop_pkts);
HFI1_DIAGC_ATTR(pkt_drops);
HFI1_DIAGC_ATTR(dmawait);
HFI1_DIAGC_ATTR(unaligned);
HFI1_DIAGC_ATTR(rc_dupreq);
HFI1_DIAGC_ATTR(rc_seqnak);

static struct attribute *diagc_default_attributes[] = {
	&diagc_attr_rc_resends.attr,
	&diagc_attr_rc_acks.attr,
	&diagc_attr_rc_qacks.attr,
	&diagc_attr_rc_delayed_comp.attr,
	&diagc_attr_seq_naks.attr,
	&diagc_attr_rdma_seq.attr,
	&diagc_attr_rnr_naks.attr,
	&diagc_attr_other_naks.attr,
	&diagc_attr_rc_timeouts.attr,
	&diagc_attr_loop_pkts.attr,
	&diagc_attr_pkt_drops.attr,
	&diagc_attr_dmawait.attr,
	&diagc_attr_unaligned.attr,
	&diagc_attr_rc_dupreq.attr,
	&diagc_attr_rc_seqnak.attr,
	NULL
};

static ssize_t diagc_attr_show(struct kobject *kobj, struct attribute *attr,
			       char *buf)
{
	struct hfi1_diagc_attr *dattr =
		container_of(attr, struct hfi1_diagc_attr, attr);
	struct hfi1_pportdata *ppd =
		container_of(kobj, struct hfi1_pportdata, diagc_kobj);
	struct hfi1_ibport *hfip = &ppd->ibport_data;

	switch (dattr->type) {
	case (HFI1_DIAGC_PCPU):
		return(sprintf(buf, "%lld\n",
			read_port_cntr(ppd,
				       dattr->counter,
				       dattr->vl)));
	case (HFI1_DIAGC_NORMAL):
		/* Fall through */
	default:
		return sprintf(buf, "%u\n",
			*(u32 *)((char *)hfip + dattr->counter));
	}
}

static ssize_t diagc_attr_store(struct kobject *kobj, struct attribute *attr,
				const char *buf, size_t size)
{
	struct hfi1_diagc_attr *dattr =
		container_of(attr, struct hfi1_diagc_attr, attr);
	struct hfi1_pportdata *ppd =
		container_of(kobj, struct hfi1_pportdata, diagc_kobj);
	struct hfi1_ibport *hfip = &ppd->ibport_data;
	u32 val;
	int ret;

	ret = kstrtou32(buf, 0, &val);
	if (ret)
		return ret;
	*(u32 *)((char *)hfip + dattr->counter) = val;
	return size;
}

static const struct sysfs_ops hfi1_diagc_ops = {
	.show = diagc_attr_show,
	.store = diagc_attr_store,
};

static struct kobj_type diagc_ktype = {
	.release = port_release,
	.sysfs_ops = &hfi1_diagc_ops,
	.default_attrs = diagc_default_attributes
};

/* End diag_counters */

/* end of per-port file structures and support code */

/*
 * Start of per-unit (or driver, in some cases, but replicated
 * per unit) functions (these get a device *)
 */
static ssize_t show_rev(struct device *device, struct device_attribute *attr,
			char *buf)
{
	struct hfi1_ibdev *dev =
		container_of(device, struct hfi1_ibdev, ibdev.dev);

	return sprintf(buf, "%x\n", dd_from_dev(dev)->minrev);
}

static ssize_t show_hca(struct device *device, struct device_attribute *attr,
			char *buf)
{
	struct hfi1_ibdev *dev =
		container_of(device, struct hfi1_ibdev, ibdev.dev);
	struct hfi_devdata *dd = dd_from_dev(dev);
	int ret;

	if (!dd->boardname)
		ret = -EINVAL;
	else
		ret = scnprintf(buf, PAGE_SIZE, "%s\n", dd->boardname);
	return ret;
}

static ssize_t show_version(struct device *device,
			    struct device_attribute *attr, char *buf)
{
	/* The string printed here is already newline-terminated. */
	return scnprintf(buf, PAGE_SIZE, "%s", (char *)ib_hfi1_version);
}

static ssize_t show_boardversion(struct device *device,
				 struct device_attribute *attr, char *buf)
{
	struct hfi1_ibdev *dev =
		container_of(device, struct hfi1_ibdev, ibdev.dev);
	struct hfi_devdata *dd = dd_from_dev(dev);

	/* The string printed here is already newline-terminated. */
	return scnprintf(buf, PAGE_SIZE, "%s", dd->boardversion);
}


static ssize_t show_localbus_info(struct device *device,
				  struct device_attribute *attr, char *buf)
{
	struct hfi1_ibdev *dev =
		container_of(device, struct hfi1_ibdev, ibdev.dev);
	struct hfi_devdata *dd = dd_from_dev(dev);

	return scnprintf(buf, PAGE_SIZE, "%s\n", dd->lbus_info);
}


static ssize_t show_nctxts(struct device *device,
			   struct device_attribute *attr, char *buf)
{
	struct hfi1_ibdev *dev =
		container_of(device, struct hfi1_ibdev, ibdev.dev);
	struct hfi_devdata *dd = dd_from_dev(dev);

	/*
	 * Return the smaller of send and receive contexts.
	 * Normally, user level applications would require both a send
	 * and a receive context, so returning the smaller of the two counts
	 * give a more accurate picture of total contexts available.
	 */
	return scnprintf(buf, PAGE_SIZE, "%u\n",
			 min(dd->num_rcv_contexts - dd->first_user_ctxt,
			     (u32)dd->sc_sizes[SC_USER].count));
}

static ssize_t show_nfreectxts(struct device *device,
			   struct device_attribute *attr, char *buf)
{
	struct hfi1_ibdev *dev =
		container_of(device, struct hfi1_ibdev, ibdev.dev);
	struct hfi_devdata *dd = dd_from_dev(dev);

	/* Return the number of free user ports (contexts) available. */
	return scnprintf(buf, PAGE_SIZE, "%u\n", dd->freectxts);
}

static ssize_t show_serial(struct device *device,
			   struct device_attribute *attr, char *buf)
{
	struct hfi1_ibdev *dev =
		container_of(device, struct hfi1_ibdev, ibdev.dev);
	struct hfi_devdata *dd = dd_from_dev(dev);

	return scnprintf(buf, PAGE_SIZE, "%s", dd->serial);

}

static ssize_t store_chip_reset(struct device *device,
				struct device_attribute *attr, const char *buf,
				size_t count)
{
	struct hfi1_ibdev *dev =
		container_of(device, struct hfi1_ibdev, ibdev.dev);
	struct hfi_devdata *dd = dd_from_dev(dev);
	int ret;

	if (count < 5 || memcmp(buf, "reset", 5) || !dd->diag_client) {
		ret = -EINVAL;
		goto bail;
	}

	ret = hfi1_reset_device(dd->unit);
bail:
	return ret < 0 ? ret : count;
}

/*
 * Convert the reported temperature from an integer (reported in
 * units of 0.25C) to a floating point number.
 */
#define temp2str(temp, buf, size, idx)					\
	scnprintf((buf) + (idx), (size) - (idx), "%u.%02u ",		\
			      ((temp) >> 2), ((temp) & 0x3) * 25)

/*
 * Dump tempsense values, in decimal, to ease shell-scripts.
 */
static ssize_t show_tempsense(struct device *device,
			      struct device_attribute *attr, char *buf)
{
	struct hfi1_ibdev *dev =
		container_of(device, struct hfi1_ibdev, ibdev.dev);
	struct hfi_devdata *dd = dd_from_dev(dev);
	struct hfi_temp temp;
	int ret = -ENXIO;

	ret = hfi1_tempsense_rd(dd, &temp);
	if (!ret) {
		int idx = 0;

		idx += temp2str(temp.curr, buf, PAGE_SIZE, idx);
		idx += temp2str(temp.lo_lim, buf, PAGE_SIZE, idx);
		idx += temp2str(temp.hi_lim, buf, PAGE_SIZE, idx);
		idx += temp2str(temp.crit_lim, buf, PAGE_SIZE, idx);
		idx += scnprintf(buf + idx, PAGE_SIZE - idx,
				"%u %u %u\n", temp.triggers & 0x1,
				temp.triggers & 0x2, temp.triggers & 0x4);
		ret = idx;
	}
	return ret;
}

/*
 * end of per-unit (or driver, in some cases, but replicated
 * per unit) functions
 */

/* start of per-unit file structures and support code */
static DEVICE_ATTR(hw_rev, S_IRUGO, show_rev, NULL);
static DEVICE_ATTR(hca_type, S_IRUGO, show_hca, NULL);
static DEVICE_ATTR(board_id, S_IRUGO, show_hca, NULL);
static DEVICE_ATTR(version, S_IRUGO, show_version, NULL);
static DEVICE_ATTR(nctxts, S_IRUGO, show_nctxts, NULL);
static DEVICE_ATTR(nfreectxts, S_IRUGO, show_nfreectxts, NULL);
static DEVICE_ATTR(serial, S_IRUGO, show_serial, NULL);
static DEVICE_ATTR(boardversion, S_IRUGO, show_boardversion, NULL);
static DEVICE_ATTR(tempsense, S_IRUGO, show_tempsense, NULL);
static DEVICE_ATTR(localbus_info, S_IRUGO, show_localbus_info, NULL);
static DEVICE_ATTR(chip_reset, S_IWUSR, NULL, store_chip_reset);

static struct device_attribute *hfi1_attributes[] = {
	&dev_attr_hw_rev,
	&dev_attr_hca_type,
	&dev_attr_board_id,
	&dev_attr_version,
	&dev_attr_nctxts,
	&dev_attr_nfreectxts,
	&dev_attr_serial,
	&dev_attr_boardversion,
	&dev_attr_tempsense,
	&dev_attr_localbus_info,
	&dev_attr_chip_reset,
};

int hfi1_create_port_files(struct ib_device *ibdev, u8 port_num,
			   struct kobject *kobj)
{
	struct hfi1_pportdata *ppd;
	struct hfi_devdata *dd = dd_from_ibdev(ibdev);
	int ret;

	if (!port_num || port_num > dd->num_pports) {
		dd_dev_err(dd,
			"Skipping infiniband class with invalid port %u\n",
			port_num);
		ret = -ENODEV;
		goto bail;
	}
	ppd = &dd->pport[port_num - 1];

	ret = kobject_init_and_add(&ppd->pport_kobj, &hfi1_port_ktype, kobj,
				   "linkcontrol");
	if (ret) {
		dd_dev_err(dd,
			"Skipping linkcontrol sysfs info, (err %d) port %u\n",
			ret, port_num);
		goto bail;
	}
	kobject_uevent(&ppd->pport_kobj, KOBJ_ADD);

	ret = kobject_init_and_add(&ppd->sc2vl_kobj, &hfi_sc2vl_ktype, kobj,
				   "sc2vl");
	if (ret) {
		dd_dev_err(dd,
			   "Skipping sc2vl sysfs info, (err %d) port %u\n",
			   ret, port_num);
		goto bail_link;
	}
	kobject_uevent(&ppd->sc2vl_kobj, KOBJ_ADD);
	ret = kobject_init_and_add(&ppd->sl2sc_kobj, &hfi_sl2sc_ktype, kobj,
				   "sl2sc");
	if (ret) {
		dd_dev_err(dd,
			   "Skipping sl2sc sysfs info, (err %d) port %u\n",
			   ret, port_num);
		goto bail_link;
	}
	kobject_uevent(&ppd->sl2sc_kobj, KOBJ_ADD);

	ret = kobject_init_and_add(&ppd->vl2mtu_kobj, &hfi_vl2mtu_ktype, kobj,
				   "vl2mtu");
	if (ret) {
		dd_dev_err(dd,
			   "Skipping vl2mtu sysfs info, (err %d) port %u\n",
			   ret, port_num);
		goto bail_sl;
	}
	kobject_uevent(&ppd->vl2mtu_kobj, KOBJ_ADD);

	ret = kobject_init_and_add(&ppd->diagc_kobj, &diagc_ktype, kobj,
				   "diag_counters");
	if (ret) {
		dd_dev_err(dd,
			"Skipping diag_counters sysfs info, (err %d) port %u\n",
			ret, port_num);
		goto bail_mtu;
	}
	kobject_uevent(&ppd->diagc_kobj, KOBJ_ADD);

	ret = kobject_init_and_add(&ppd->pport_cc_kobj, &port_cc_ktype,
				   kobj, "CCMgtA");
	if (ret) {
		dd_dev_err(dd,
		 "Skipping Congestion Control sysfs info, (err %d) port %u\n",
		 ret, port_num);
		goto bail_diagc;
	}

	kobject_uevent(&ppd->pport_cc_kobj, KOBJ_ADD);

	ret = sysfs_create_bin_file(&ppd->pport_cc_kobj,
				&cc_setting_bin_attr);
	if (ret) {
		dd_dev_err(dd,
		 "Skipping Congestion Control setting sysfs info, (err %d) port %u\n",
		 ret, port_num);
		goto bail_cc;
	}

	ret = sysfs_create_bin_file(&ppd->pport_cc_kobj,
				&cc_table_bin_attr);
	if (ret) {
		dd_dev_err(dd,
		 "Skipping Congestion Control table sysfs info, (err %d) port %u\n",
		 ret, port_num);
		goto bail_cc_entry_bin;
	}

	dd_dev_info(dd,
		"IB%u: Congestion Control Agent enabled for port %d\n",
		dd->unit, port_num);

	return 0;

bail_cc_entry_bin:
	sysfs_remove_bin_file(&ppd->pport_cc_kobj, &cc_setting_bin_attr);
bail_cc:
	kobject_put(&ppd->pport_cc_kobj);
bail_diagc:
	kobject_put(&ppd->diagc_kobj);
bail_mtu:
	kobject_put(&ppd->vl2mtu_kobj);
bail_sl:
	kobject_put(&ppd->sc2vl_kobj);
bail_link:
	kobject_put(&ppd->pport_kobj);
bail:
	return ret;
}

/*
 * Register and create our files in /sys/class/infiniband.
 */
int hfi1_verbs_register_sysfs(struct hfi_devdata *dd)
{
	struct ib_device *dev = &dd->verbs_dev.ibdev;
	int i, ret;

	for (i = 0; i < ARRAY_SIZE(hfi1_attributes); ++i) {
		ret = device_create_file(&dev->dev, hfi1_attributes[i]);
		if (ret)
			goto bail;
	}

	return 0;
bail:
	for (i = 0; i < ARRAY_SIZE(hfi1_attributes); ++i)
		device_remove_file(&dev->dev, hfi1_attributes[i]);
	return ret;
}

/*
 * Unregister and remove our files in /sys/class/infiniband.
 */
void hfi1_verbs_unregister_sysfs(struct hfi_devdata *dd)
{
	struct hfi1_pportdata *ppd;
	int i;

	for (i = 0; i < dd->num_pports; i++) {
		ppd = &dd->pport[i];

		sysfs_remove_bin_file(&ppd->pport_cc_kobj,
			&cc_setting_bin_attr);
		sysfs_remove_bin_file(&ppd->pport_cc_kobj,
			&cc_table_bin_attr);
		kobject_put(&ppd->pport_cc_kobj);
		kobject_put(&ppd->vl2mtu_kobj);
		kobject_put(&ppd->sc2vl_kobj);
		kobject_put(&ppd->pport_kobj);
	}
}
