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
#include "nd.h"

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
		.out_num = 2,
		.out_sizes = { 4, 8, },
	},
	[NFIT_CMD_GET_CONFIG_SIZE] = {
		.out_num = 3,
		.out_sizes = { 4, 4, 4, },
	},
	[NFIT_CMD_GET_CONFIG_DATA] = {
		.in_num = 2,
		.in_sizes = { 4, 4, },
		.out_num = 2,
		.out_sizes = { 4, UINT_MAX, },
	},
	[NFIT_CMD_SET_CONFIG_DATA] = {
		.in_num = 3,
		.in_sizes = { 4, 4, UINT_MAX, },
		.out_num = 1,
		.out_sizes = { 4, },
	},
	[NFIT_CMD_VENDOR] = {
		.in_num = 2,
		.in_sizes = { 4, UINT_MAX, },
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
		.out_num = 1,
		.out_sizes = { 4, },
	},
	[NFIT_CMD_SMART_THRESHOLD] = {
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
		struct nd_dimm *nd_dimm, unsigned int cmd, void *buf,
		unsigned int buf_len)
{
	const struct cmd_desc *desc = to_cmd_desc(cmd);
	union acpi_object in_obj, in_buf, *out_obj;
	unsigned long dsm_mask;
	acpi_handle handle;
	struct device *dev;
	int rc, i;
	u32 offset;

	if (!desc)
		return -ENOTTY;

	if (nd_dimm) {
		struct acpi_device *adev = nd_dimm->priv_data;

		dsm_mask = nd_dimm->dsm_mask;
		handle = adev->handle;
		dev = &adev->dev;
	} else {
		struct acpi_nfit *nfit = to_acpi_nfit(nfit_desc);

		dsm_mask = nfit_desc->dsm_mask;
		handle = nfit->dev->handle;
		dev = &nfit->dev->dev;
	}

	if (!test_bit(cmd, &dsm_mask))
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

	dev_dbg(dev, "%s: cmd: %s input length: %d\n", __func__,
			nfit_cmd_name(cmd), in_buf.buffer.length);
	if (IS_ENABLED(CONFIG_NFIT_ACPI_DEBUG))
		print_hex_dump_debug(nfit_cmd_name(cmd), DUMP_PREFIX_OFFSET, 4,
				4, in_buf.buffer.pointer, min_t(u32, 128,
					in_buf.buffer.length), true);

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
	if (IS_ENABLED(CONFIG_NFIT_ACPI_DEBUG))
		print_hex_dump_debug(nfit_cmd_name(cmd), DUMP_PREFIX_OFFSET, 4,
				4, out_obj->buffer.pointer, min_t(u32, 128,
					out_obj->buffer.length), true);

	for (i = 0, offset = 0; i < desc->out_num; i++) {
		u32 out_size = to_cmd_out_size(cmd, desc, i, buf,
				out_obj->buffer.length, offset);

		if (out_size == UINT_MAX) {
			dev_dbg(dev, "%s: unknown output size cmd: %s field: %d\n",
					__func__, nfit_cmd_name(cmd), i);
			break;
		}

		if (offset + out_size > out_obj->buffer.length) {
			dev_dbg(dev, "%s: output object underflow cmd: %s field: %d\n",
					__func__, nfit_cmd_name(cmd), i);
			break;
		}

		if (in_buf.buffer.length + offset + out_size > buf_len) {
			dev_dbg(dev, "%s: output overrun cmd: %s field: %d\n",
					__func__, nfit_cmd_name(cmd), i);
			rc = -ENXIO;
			goto out;
		}
		memcpy(buf + in_buf.buffer.length + offset,
				out_obj->buffer.pointer + offset, out_size);
		offset += out_size;
	}
	if (offset + in_buf.buffer.length < buf_len) {
		if (i >= 1) {
			/*
			 * status valid, return the number of bytes left
			 * unfilled in the output buffer
			 */
			rc = buf_len - offset;
		} else {
			dev_err(dev, "%s: underrun cmd: %s buf_len: %d out_len: %d\n",
					__func__, nfit_cmd_name(cmd), buf_len, offset);
			rc = -ENXIO;
		}
	} else
		rc = 0;

 out:
	ACPI_FREE(out_obj);

	return rc;
}

static int nd_acpi_add_dimm(struct nfit_bus_descriptor *nfit_desc,
		struct nd_dimm *nd_dimm)
{
	struct acpi_nfit *nfit = to_acpi_nfit(nfit_desc);
	u32 nfit_handle = to_nfit_handle(nd_dimm);
	struct acpi_device *acpi_dimm;
	int i;

	acpi_dimm = acpi_find_child_device(nfit->dev, nfit_handle, false);
	if (!acpi_dimm)
		return -ENODEV;

	for (i = NFIT_CMD_SMART; i <= NFIT_CMD_SMART_THRESHOLD; i++)
		if (acpi_check_dsm(acpi_dimm->handle, nd_acpi_uuid, 1, 1ULL << i))
			set_bit(i, &nd_dimm->dsm_mask);

	nd_dimm->priv_data = acpi_dimm;
	return 0;
}

static int nd_acpi_add(struct acpi_device *dev)
{
	struct nfit_bus_descriptor *nfit_desc;
	struct acpi_table_header *tbl;
	acpi_status status = AE_OK;
	struct acpi_nfit *nfit;
	acpi_size sz;
	int i;

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
	nfit_desc->add_dimm = nd_acpi_add_dimm;

	for (i = NFIT_CMD_SMART; i <= NFIT_CMD_SMART_THRESHOLD; i++)
		if (acpi_check_dsm(dev->handle, nd_acpi_uuid, 1, 1ULL << i))
			set_bit(i, &nfit_desc->dsm_mask);

	nfit->nd_bus = nfit_bus_register(&dev->dev, nfit_desc);
	if (!nfit->nd_bus)
		return -ENXIO;

	dev_set_drvdata(&dev->dev, nfit);
	return 0;
}

static int nd_acpi_remove(struct acpi_device *dev)
{
	struct acpi_nfit *nfit = dev_get_drvdata(&dev->dev);

	nfit_bus_unregister(nfit->nd_bus);
	return 0;
}

static void nd_acpi_notify(struct acpi_device *dev, u32 event)
{
	/* TODO: handle ACPI_NOTIFY_BUS_CHECK notification */
	dev_dbg(&dev->dev, "%s: event: %d\n", __func__, event);
}

static const struct acpi_device_id nd_acpi_ids[] = {
	{ "ACPI0012", 0 },
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

static __init int nd_acpi_init(void)
{
	if (acpi_str_to_uuid("4309ac30-0d11-11e4-9191-0800200c9a66",
				nd_acpi_uuid) != AE_OK) {
		WARN_ON_ONCE(1);
		return -ENXIO;
	}

	return acpi_bus_register_driver(&nd_acpi_driver);
}

static __exit void nd_acpi_exit(void)
{
	acpi_bus_unregister_driver(&nd_acpi_driver);
}

module_init(nd_acpi_init);
module_exit(nd_acpi_exit);
MODULE_LICENSE("GPL v2");
MODULE_AUTHOR("Intel Corporation");
