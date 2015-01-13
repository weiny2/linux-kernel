/*
 * Copyright(c) 2013-2014 Intel Corporation. All rights reserved.
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
#include <linux/list.h>
#include <linux/acpi.h>
#include <linux/mutex.h>
#include <linux/ndctl.h>
#include <linux/module.h>
#include "nfit.h"
#include "dsm.h" /* nd_manual_dsm */

static bool old_acpi = true;
module_param(old_acpi, bool, 0);
MODULE_PARM_DESC(old_acpi, "Search for NFIT by _CRS on _HID(ACPI0010)");

unsigned long force_dsm = 0;
module_param(force_dsm, long, S_IRUGO | S_IWUSR);
MODULE_PARM_DESC(force_dsm, "Force enable ACPI DSMs");

enum {
	NFIT_ACPI_NOTIFY_TABLE = 0x80,
};

struct acpi_nfit {
	struct nfit_bus_descriptor nfit_desc;
	struct acpi_device *dev;
	struct nd_bus *nd_bus;
};

static struct acpi_nfit *to_acpi_nfit(struct nfit_bus_descriptor *nfit_desc)
{
	return container_of(nfit_desc, struct acpi_nfit, nfit_desc);
}

#define NFIT_ACPI_MAX_ELEM 4
struct cmd_desc {
	int in_num;
	int out_num;
	u32 in_sizes[NFIT_ACPI_MAX_ELEM];
	int out_sizes[NFIT_ACPI_MAX_ELEM];
};

static const struct cmd_desc nfit_acpi_descs[] = {
	[NFIT_CMD_IMPLEMENTED] = { },
	[NFIT_CMD_SMART] = {
		.in_num = 1,
		.out_num = 2,
		.in_sizes = { 4, },
		.out_sizes = { 4, 8, },
	},
	[NFIT_CMD_GET_CONFIG_SIZE] = {
		.in_num = 1,
		.in_sizes = { 4, },
		.out_num = 3,
		.out_sizes = { 4, 4, 4, },
	},
	[NFIT_CMD_GET_CONFIG_DATA] = {
		.in_num = 3,
		.in_sizes = { 4, 4, 4, },
		.out_num = 2,
		.out_sizes = { 4, UINT_MAX, },
	},
	[NFIT_CMD_SET_CONFIG_DATA] = {
		.in_num = 4,
		.in_sizes = { 4, 4, 4, UINT_MAX, },
		.out_num = 1,
		.out_sizes = { 4, },
	},
	[NFIT_CMD_VENDOR] = {
		.in_num = 3,
		.in_sizes = { 4, 4, UINT_MAX, },
		.out_num = 3,
		.out_sizes = { 4, 4, UINT_MAX, },
	},
	[NFIT_CMD_ARS_CAP] = {
		.in_num = 2,
		.in_sizes = { 8, 8, },
		.out_num = 1,
		.out_sizes = { 4, },
	},
	[NFIT_CMD_ARS_START] = {
		.in_num = 3,
		.in_sizes = { 8, 8, 2, },
		.out_num = 1,
		.out_sizes = { 4, },
	},
	[NFIT_CMD_ARS_QUERY] = {
		.out_num = 2,
		.out_sizes = { 4, UINT_MAX, },
	},
	[NFIT_CMD_ARM] = {
		.in_num = 1,
		.in_sizes = { 4, },
		.out_num = 1,
		.out_sizes = { 4, },
	},
	[NFIT_CMD_SMART_THRESHOLD] = {
		.in_num = 1,
		.in_sizes = { 4, },
		.out_num = 2,
		.out_sizes = { 4, 8, },
	},
};

static const struct cmd_desc *to_cmd_desc(unsigned int cmd)
{
	if (cmd <= NFIT_CMD_ARM && cmd >= NFIT_CMD_SMART)
		return &nfit_acpi_descs[cmd];
	return NULL;
}

static u32 to_cmd_in_size(int cmd, const struct cmd_desc *desc, int idx,
		void *buf)
{
	if (idx >= desc->in_num)
		return UINT_MAX;

	if (desc->in_sizes[idx] < UINT_MAX)
		return desc->in_sizes[idx];

	if (cmd == NFIT_CMD_SET_CONFIG_DATA && idx == 3) {
		struct nfit_cmd_set_config_hdr *hdr = buf;

		return hdr->in_length;
	} else if (cmd == NFIT_CMD_VENDOR && idx == 2) {
		struct nfit_cmd_vendor_hdr *hdr = buf;

		return hdr->in_length;
	}

	return UINT_MAX;
}

static u32 to_cmd_out_size(int cmd, const struct cmd_desc *desc, int idx,
		void *buf, u32 out_length, u32 offset)
{
	if (idx >= desc->out_num)
		return UINT_MAX;

	if (desc->out_sizes[idx] < UINT_MAX)
		return desc->out_sizes[idx];

	if (cmd == NFIT_CMD_GET_CONFIG_DATA && idx == 1) {
		struct nfit_cmd_get_config_data_hdr *hdr = buf;

		return hdr->in_length;
	} else if (cmd == NFIT_CMD_VENDOR && idx == 2
			&& offset < out_length) {
		return out_length - offset;
	} else if (cmd == NFIT_CMD_ARS_QUERY && idx == 1
			&& offset < out_length) {
		return out_length - offset;
	}

	return UINT_MAX;
}

static u8 nd_acpi_uuid[16]; /* initialized at nd_acpi_init */

static int nd_acpi_ctl(struct nfit_bus_descriptor *nfit_desc,
		unsigned int cmd, void *buf, unsigned int buf_len)
{
	struct acpi_nfit *nfit = to_acpi_nfit(nfit_desc);
	const struct cmd_desc *desc = to_cmd_desc(cmd);
	union acpi_object in_obj, in_buf, *out_obj;
	acpi_handle handle = nfit->dev->handle;
	struct device *dev = &nfit->dev->dev;
	int rc = 0, i;
	u32 offset;

	if (test_bit(cmd, &nd_manual_dsm))
		return nd_dsm_ctl(nfit_desc, cmd, buf, buf_len);

	if (!desc)
		return -ENOTTY;

	if (!test_bit(cmd, &nfit_desc->dsm_mask))
		return -ENOTTY;

	in_obj.type = ACPI_TYPE_PACKAGE;
	in_obj.package.count = 1;
	in_obj.package.elements = &in_buf;
	in_buf.type = ACPI_TYPE_BUFFER;
	in_buf.buffer.pointer = buf;
	in_buf.buffer.length = 0;

	/* double check that the nfit_acpi_cmd_descs table is self consistent */
	if (desc->in_num > NFIT_ACPI_MAX_ELEM) {
		WARN_ON_ONCE(1);
		return -ENXIO;
	}

	for (i = 0; i < desc->in_num; i++) {
		u32 in_size;

		in_size = to_cmd_in_size(cmd, desc, i, buf);
		if (in_size == UINT_MAX) {
			dev_err(dev, "%s: unknown input size cmd: %s field: %d\n",
					__func__, nfit_cmd_name(cmd), i);
			return -ENXIO;
		}
		in_buf.buffer.length += in_size;
		if (in_buf.buffer.length > buf_len) {
			dev_err(dev, "%s: input underrun cmd: %s field: %d\n",
					__func__, nfit_cmd_name(cmd), i);
			return -ENXIO;
		}
	}

	out_obj = acpi_evaluate_dsm(handle, nd_acpi_uuid, 1, cmd, &in_obj);
	if (!out_obj) {
		dev_dbg(dev, "%s: _DSM failed cmd: %s\n", __func__,
				nfit_cmd_name(cmd));
		return -EINVAL;
	}

	if (out_obj->package.type != ACPI_TYPE_BUFFER) {
		dev_dbg(dev, "%s: unexpected output object type cmd: %s type: %d\n",
				__func__, nfit_cmd_name(cmd), out_obj->type);
		rc = -EINVAL;
		goto out;
	}
	dev_dbg(dev, "%s: cmd: %s output length: %d\n", __func__,
			nfit_cmd_name(cmd), out_obj->buffer.length);
	if (IS_ENABLED(CONFIG_DYNAMIC_DEBUG))
		print_hex_dump_debug(nfit_cmd_name(cmd), DUMP_PREFIX_OFFSET, 16,
				1, out_obj->buffer.pointer, min_t(u32, 128,
					out_obj->buffer.length), true);

	for (i = 0, offset = 0; i < desc->out_num; i++) {
		u32 out_size = to_cmd_out_size(cmd, desc, i, buf,
				out_obj->buffer.length, offset);

		if (out_size == UINT_MAX) {
			dev_err(dev, "%s: unknown output size cmd: %s field: %d\n",
					__func__, nfit_cmd_name(cmd), i);
			rc = -ENXIO;
			goto out;
		}

		if (offset + out_size > out_obj->buffer.length) {
			dev_err(dev, "%s: output object underflow cmd: %s field: %d\n",
					__func__, nfit_cmd_name(cmd), i);
			rc = -ENXIO;
			goto out;
		}

		if (in_buf.buffer.length + offset + out_size > buf_len) {
			dev_err(dev, "%s: output overrun cmd: %s field: %d\n",
					__func__, nfit_cmd_name(cmd), i);
			rc = -ENXIO;
			goto out;
		}
		memcpy(buf + in_buf.buffer.length + offset,
				out_obj->buffer.pointer + offset, out_size);
		offset += out_size;
	}
	if (offset + in_buf.buffer.length < buf_len) {
		if (cmd == NFIT_CMD_VENDOR || cmd == NFIT_CMD_ARS_QUERY) {
			rc = buf_len - offset; /* part of return buffer is invalid */
		} else {
			dev_err(dev, "%s: underrun cmd: %s buf_len: %d out_len: %d\n",
					__func__, nfit_cmd_name(cmd), buf_len, offset);
			rc = -ENXIO;
		}
	}
 out:
	ACPI_FREE(out_obj);

	return rc;
}

static struct acpi_device *nd_acpi_root;

static int nd_acpi_add(struct acpi_device *dev)
{
	struct nfit_bus_descriptor *nfit_desc;
	struct acpi_table_header *tbl;
	acpi_status status = AE_OK;
	struct acpi_nfit *nfit;
	acpi_size sz;
	int i;

	if (nd_acpi_root) {
		dev_info(&dev->dev, "ignoring, only one root device supported\n");
		return -EINVAL;
	}

	status = acpi_get_table_with_size("NFIT", 0, &tbl, &sz);
	if (ACPI_FAILURE(status)) {
		dev_err(&dev->dev, "failed to find NFIT\n");
		return -ENXIO;
	}

	nfit = devm_kzalloc(&dev->dev, sizeof(*nfit), GFP_KERNEL);
	if (!nfit)
		return -ENOMEM;
	nfit->dev = dev;
	nfit_desc = &nfit->nfit_desc;
	nfit_desc->nfit_base = (void __iomem *) tbl;
	nfit_desc->nfit_size = sz;
	nfit_desc->provider_name = "ACPI.NFIT";
	nfit_desc->nfit_ctl = nd_acpi_ctl;

	for (i = NFIT_CMD_SMART; i <= NFIT_CMD_SMART_THRESHOLD; i++) {
		if (acpi_check_dsm(dev->handle, nd_acpi_uuid, 1, 1ULL << i))
			set_bit(i, &nfit_desc->dsm_mask);
	}

	/* ACPI references one global NFIT for all devices, i.e. no parent */
	nfit->nd_bus = nfit_bus_register(NULL, nfit_desc);
	if (!nfit->nd_bus)
		return -ENXIO;

	dev_set_drvdata(&dev->dev, nfit);
	nd_acpi_root = dev;
	return 0;
}

static int nd_acpi_remove(struct acpi_device *dev)
{
	struct acpi_nfit *nfit = dev_get_drvdata(&dev->dev);

	nfit_bus_unregister(nfit->nd_bus);
	nd_acpi_root = NULL;
	return 0;
}

static void nd_acpi_notify(struct acpi_device *dev, u32 event)
{
	/* TODO: handle ACPI_NOTIFY_BUS_CHECK notification */
	dev_dbg(&dev->dev, "%s: event: %d\n", __func__, event);
}

static const struct acpi_device_id nd_acpi_ids[] = {
	{ "ACPI0011", 0 },
	{ "", 0 },
};
MODULE_DEVICE_TABLE(acpi, nd_acpi_ids);

static struct acpi_driver nd_acpi_driver = {
	.name = KBUILD_MODNAME,
	.ids = nd_acpi_ids,
	.flags = ACPI_DRIVER_ALL_NOTIFY_EVENTS,
	.ops = {
		.add = nd_acpi_add,
		.remove = nd_acpi_remove,
		.notify = nd_acpi_notify
	},
};

static const struct acpi_device_id nd_acpi_dimm_ids[] = {
	{ "ACPI0010", 0 },
	{ "", 0 },
};
MODULE_DEVICE_TABLE(acpi, nd_acpi_dimm_ids);

static acpi_status legacy_nd_acpi_add_nfit(struct acpi_resource *resource,
		void *_dev)
{
	struct acpi_resource_address64 address64;
	struct nfit_bus_descriptor *nfit_desc;
	struct acpi_device *dev = _dev;
	struct acpi_nfit *nfit;
	resource_size_t start;
	acpi_status status;

	status = acpi_resource_to_address64(resource, &address64);
	if (ACPI_FAILURE(status))
		return AE_CTRL_TERMINATE;

	start = address64.minimum;
	nfit = devm_kzalloc(&dev->dev, sizeof(*nfit), GFP_KERNEL);
	if (!nfit)
		return AE_ERROR;
	nfit->dev = dev;
	nfit_desc = &nfit->nfit_desc;
	nfit_desc->nfit_size = address64.address_length;
	nfit_desc->nfit_base = ioremap_cache(start, nfit_desc->nfit_size);
	nfit_desc->old_nfit = true;
	if (!nfit_desc->nfit_base)
		return AE_ERROR;
	nfit_desc->provider_name = "ACPI.NFIT";
	nfit_desc->nfit_ctl = nd_acpi_ctl;
	nfit_desc->dsm_mask |= nd_manual_dsm | force_dsm;

	/* ACPI references one global NFIT for all devices, i.e. no parent */
	nfit->nd_bus = nfit_bus_register(NULL, nfit_desc);
	if (!nfit->nd_bus)
		return AE_ERROR;
	else {
		dev_set_drvdata(&dev->dev, nfit);
		return AE_OK;
	}
}

static int legacy_nd_acpi_add(struct acpi_device *dev)
{
	acpi_status status;

	status = acpi_walk_resources(dev->handle, METHOD_NAME__CRS,
				     legacy_nd_acpi_add_nfit, dev);
	if (ACPI_FAILURE(status))
		return -EINVAL;

	return 0;
}

static int try_legacy_discovery(struct acpi_device *dev)
{
	int rc = legacy_nd_acpi_add(dev);

	if (rc == 0) {
		dev_err(&dev->dev, "%s: discovering NFIT by _CRS is deprecated\n",
				__func__);
		add_taint(TAINT_FIRMWARE_WORKAROUND, LOCKDEP_STILL_OK);
	}

	return rc;
}

static int nd_acpi_dimm_add(struct acpi_device *dev)
{
	if (!nd_acpi_root) {
		dev_err(&dev->dev, "missing ACPI.NFIT root device\n");
		if (!old_acpi)
			return -ENXIO;
		else
			return try_legacy_discovery(dev);
	}

	/* TODO: add _FIT parsing */

	return 0;
}

static int nd_acpi_dimm_remove(struct acpi_device *dev)
{
	if (old_acpi) {
		struct acpi_nfit *nfit = dev_get_drvdata(&dev->dev);
		struct nfit_bus_descriptor *nfit_desc = &nfit->nfit_desc;

		nfit_bus_unregister(nfit->nd_bus);
		iounmap(nfit_desc->nfit_base);
	}

	return 0;
}

static void nd_acpi_dimm_notify(struct acpi_device *dev, u32 event)
{
	/* TODO: handle NFIT_ACPI_NOTIFY_TABLE notification */
	dev_dbg(&dev->dev, "%s: event: %d\n", __func__, event);
}

static struct acpi_driver nd_acpi_dimm_driver = {
	.name = KBUILD_MODNAME "_dimm",
	.ids = nd_acpi_dimm_ids,
	.flags = ACPI_DRIVER_ALL_NOTIFY_EVENTS,
	.ops = {
		.add = nd_acpi_dimm_add,
		.remove = nd_acpi_dimm_remove,
		.notify = nd_acpi_dimm_notify
	},
};

static __init int nd_acpi_init(void)
{
	int rc;

	if (acpi_str_to_uuid("4309ac30-0d11-11e4-9191-0800200c9a66",
				nd_acpi_uuid) != AE_OK) {
		WARN_ON_ONCE(1);
		return -ENXIO;
	}

	rc = acpi_bus_register_driver(&nd_acpi_driver);
	if (rc)
		return rc;
	rc = acpi_bus_register_driver(&nd_acpi_dimm_driver);
	if (rc) {
		acpi_bus_unregister_driver(&nd_acpi_driver);
		return rc;
	}
	return 0;
}

static __exit void nd_acpi_exit(void)
{
	acpi_bus_unregister_driver(&nd_acpi_dimm_driver);
	acpi_bus_unregister_driver(&nd_acpi_driver);
}

module_init(nd_acpi_init);
module_exit(nd_acpi_exit);
MODULE_LICENSE("GPL v2");
MODULE_AUTHOR("Intel Corporation");
