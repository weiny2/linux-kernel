// SPDX-License-Identifier: GPL-2.0
/**
 * USB Typec-C Thuderbolt3 Alternate Mode driver
 *
 * Copyright (C) 2019 Intel Corporation
 * Author: Heikki Krogerus <heikki.krogerus@linux.intel.com>
 */

#include <linux/delay.h>
#include <linux/mutex.h>
#include <linux/module.h>
#include <linux/usb/pd_vdo.h>
#include <linux/usb/typec_tbt.h>

enum tbt_state {
	TBT_STATE_IDLE,
	TBT_STATE_SOP_P_ENTER,
	TBT_STATE_SOP_PP_ENTER,
	TBT_STATE_ENTER,
	TBT_STATE_EXIT,
	TBT_STATE_SOP_PP_EXIT,
	TBT_STATE_SOP_P_EXIT
};

struct tbt_altmode {
	enum tbt_state state;
	struct typec_cable *cable;
	struct typec_altmode *alt;
	struct typec_altmode *plug[2];

	struct work_struct work;
	struct mutex lock; /* device lock */
};

static int tbt_enter_mode(struct tbt_altmode *tbt)
{
	struct typec_altmode *plug = tbt->plug[TYPEC_PLUG_SOP_P];
	u32 vdo;

	vdo = tbt->alt->vdo & (TBT_VENDOR_SPECIFIC_B0 | TBT_VENDOR_SPECIFIC_B1);
	vdo |= tbt->alt->vdo & TBT_INTEL_SPECIFIC_B0;
	vdo |= TBT_MODE;

	if (plug) {
		if (typec_cable_is_active(tbt->cable))
			vdo |= TBT_ENTER_MODE_ACTIVE_CABLE;

		vdo |= TBT_ENTER_MODE_CABLE_SPEED(TBT_CABLE_SPEED(plug->vdo));
		vdo |= plug->vdo & TBT_CABLE_ROUNDED;
		vdo |= plug->vdo & TBT_CABLE_OPTICAL;
		vdo |= plug->vdo & TBT_CABLE_RETIMER;
		vdo |= plug->vdo & TBT_CABLE_LINK_TRAINING;
	} else {
		vdo |= TBT_ENTER_MODE_CABLE_SPEED(TBT_CABLE_USB3_PASSIVE);
	}

	return typec_altmode_enter(tbt->alt, &vdo);
}

static void tbt_altmode_work(struct work_struct *work)
{
	struct tbt_altmode *tbt = container_of(work, struct tbt_altmode, work);
	int ret;

	mutex_lock(&tbt->lock);

	switch (tbt->state) {
	case TBT_STATE_SOP_P_ENTER:
		ret = typec_altmode_enter(tbt->plug[TYPEC_PLUG_SOP_P], NULL);
		if (ret)
			dev_dbg(&tbt->plug[TYPEC_PLUG_SOP_P]->dev,
				"failed to enter mode (%d)\n", ret);
		break;
	case TBT_STATE_SOP_PP_ENTER:
		ret = typec_altmode_enter(tbt->plug[TYPEC_PLUG_SOP_PP], NULL);
		if (ret)
			dev_dbg(&tbt->plug[TYPEC_PLUG_SOP_PP]->dev,
				"failed to enter mode (%d)\n", ret);
		break;
	case TBT_STATE_ENTER:
		ret = tbt_enter_mode(tbt);
		if (ret)
			dev_dbg(&tbt->alt->dev, "failed to enter mode (%d)\n",
				ret);
		break;
	case TBT_STATE_EXIT:
		typec_altmode_exit(tbt->alt);
		break;
	case TBT_STATE_SOP_PP_EXIT:
		typec_altmode_exit(tbt->plug[TYPEC_PLUG_SOP_PP]);
		break;
	case TBT_STATE_SOP_P_EXIT:
		typec_altmode_exit(tbt->plug[TYPEC_PLUG_SOP_P]);
		break;
	default:
		break;
	}

	tbt->state = TBT_STATE_IDLE;

	mutex_unlock(&tbt->lock);
}

static int tbt_altmode_vdm(struct typec_altmode *alt,
			   const u32 hdr, const u32 *vdo, int count)
{
	struct tbt_altmode *tbt = typec_altmode_get_drvdata(alt);
	int cmd_type = PD_VDO_CMDT(hdr);
	int cmd = PD_VDO_CMD(hdr);

	mutex_lock(&tbt->lock);

	if (tbt->state != TBT_STATE_IDLE) {
		mutex_unlock(&tbt->lock);
		return -EBUSY;
	}

	switch (cmd_type) {
	case CMDT_RSP_ACK:
		switch (cmd) {
		case CMD_ENTER_MODE:
			/*
			 * Following the order describeded in USB Type-C Spec
			 * R2.0 Section 6.7.3.
			 */
			if (alt == tbt->plug[TYPEC_PLUG_SOP_P]) {
				if (tbt->plug[TYPEC_PLUG_SOP_PP])
					tbt->state = TBT_STATE_SOP_PP_ENTER;
				else
					tbt->state = TBT_STATE_ENTER;
			} else if (alt == tbt->plug[TYPEC_PLUG_SOP_PP]) {
				tbt->state = TBT_STATE_ENTER;
			} else {
				typec_altmode_notify(alt, TYPEC_STATE_MODAL,
						     NULL);
			}
			break;
		case CMD_EXIT_MODE:
			if (alt == tbt->alt) {
				if (tbt->plug[TYPEC_PLUG_SOP_PP])
					tbt->state = TBT_STATE_SOP_PP_EXIT;
				else if (tbt->plug[TYPEC_PLUG_SOP_P])
					tbt->state = TBT_STATE_SOP_P_EXIT;
			} else if (alt == tbt->plug[TYPEC_PLUG_SOP_PP]) {
				tbt->state = TBT_STATE_SOP_P_EXIT;
			}
			break;
		}
		break;
	case CMDT_RSP_NAK:
		switch (cmd) {
		case CMD_ENTER_MODE:
			dev_warn(&alt->dev, "Enter Mode refused\n");
			break;
		default:
			break;
		}
		break;
	default:
		break;
	}

	if (tbt->state != TBT_STATE_IDLE)
		schedule_work(&tbt->work);

	mutex_unlock(&tbt->lock);

	return 0;
}

static int tbt_altmode_activate(struct typec_altmode *alt, int activate)
{
	struct tbt_altmode *tbt = typec_altmode_get_drvdata(alt);
	int ret;

	mutex_lock(&tbt->lock);

	/* Preventing the user space from entering/exiting the cable alt mode */
	if (alt != tbt->alt)
		ret = -EPERM;
	else if (activate)
		ret = tbt_enter_mode(tbt);
	else
		ret = typec_altmode_exit(alt);

	mutex_unlock(&tbt->lock);

	return ret;
}

static const struct typec_altmode_ops tbt_altmode_ops = {
	.vdm		= tbt_altmode_vdm,
	.activate	= tbt_altmode_activate
};

static int tbt_altmode_probe(struct typec_altmode *alt)
{
	struct typec_altmode *plug;
	struct tbt_altmode *tbt;
	int err;
	int i;

	tbt = devm_kzalloc(&alt->dev, sizeof(*tbt), GFP_KERNEL);
	if (!tbt)
		return -ENOMEM;

	/* Thundebolt 3 requires a cable with eMarker */
	tbt->cable = typec_cable_get(typec_altmode2port(alt));
	if (!tbt->cable)
		return -ENODEV;

	for (i = 0; i < TYPEC_PLUG_SOP_PP + 1; i++) {
		plug = typec_altmode_get_plug(alt, i);
		if (IS_ERR(plug)) {
			err = PTR_ERR(plug);
			goto err_put_plugs;
		}

		if (!plug || plug->svid != USB_TYPEC_VENDOR_INTEL)
			break;

		plug->desc = "Thunderbolt3";
		plug->ops = &tbt_altmode_ops;
		typec_altmode_set_drvdata(plug, tbt);

		tbt->plug[i] = plug;
	}

	INIT_WORK(&tbt->work, tbt_altmode_work);
	mutex_init(&tbt->lock);
	tbt->alt = alt;

	alt->desc = "Thunderbolt3";
	alt->ops = &tbt_altmode_ops;

	typec_altmode_set_drvdata(alt, tbt);

	if (tbt->plug[TYPEC_PLUG_SOP_PP])
		tbt->state = TBT_STATE_SOP_PP_ENTER;
	else if (tbt->plug[TYPEC_PLUG_SOP_P])
		tbt->state = TBT_STATE_SOP_P_ENTER;
	else
		tbt->state = TBT_STATE_ENTER;
	schedule_work(&tbt->work);

	return 0;

err_put_plugs:
	typec_altmode_put_plug(tbt->plug[TYPEC_PLUG_SOP_PP]);
	typec_altmode_put_plug(tbt->plug[TYPEC_PLUG_SOP_P]);
	typec_cable_put(tbt->cable);

	return err;
}

static void tbt_altmode_remove(struct typec_altmode *alt)
{
	struct tbt_altmode *tbt = typec_altmode_get_drvdata(alt);

	typec_altmode_put_plug(tbt->plug[TYPEC_PLUG_SOP_PP]);
	typec_altmode_put_plug(tbt->plug[TYPEC_PLUG_SOP_P]);
	typec_cable_put(tbt->cable);
}

static const struct typec_device_id tbt_typec_id[] = {
	{ USB_TYPEC_VENDOR_INTEL, TYPEC_ANY_MODE },
	{ }
};
MODULE_DEVICE_TABLE(typec, tbt_typec_id);

static struct typec_altmode_driver tbt_altmode_driver = {
	.id_table = tbt_typec_id,
	.probe = tbt_altmode_probe,
	.remove = tbt_altmode_remove,
	.driver = {
		.name = "typec-thunderbolt",
		.owner = THIS_MODULE,
	}
};
module_typec_altmode_driver(tbt_altmode_driver);

MODULE_AUTHOR("Heikki Krogerus <heikki.krogerus@linux.intel.com>");
MODULE_LICENSE("GPL v2");
MODULE_DESCRIPTION("Thunderbolt3 USB Type-C Alternate Mode");
