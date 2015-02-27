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
#include <linux/mm.h>
#include <linux/list.h>
#include <linux/acpi.h>
#include <linux/mutex.h>
#include <linux/ndctl.h>
#include <linux/module.h>
#include <linux/vmalloc.h>
#include "nfit.h"
#include "dsm.h" /* nd_manual_dsm */
#include "nd.h"

unsigned long force_dsm = 0x270;
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
	[NFIT_OLD_SMART] = {
		.in_num = 1,
		.out_num = 2,
		.in_sizes = { 4, },
		.out_sizes = { 4, 8, },
	},
	[NFIT_OLD_GET_CONFIG_SIZE] = {
		.in_num = 1,
		.in_sizes = { 4, },
		.out_num = 3,
		.out_sizes = { 4, 4, 4, },
	},
	[NFIT_OLD_GET_CONFIG_DATA] = {
		.in_num = 3,
		.in_sizes = { 4, 4, 4, },
		.out_num = 2,
		.out_sizes = { 4, UINT_MAX, },
	},
	[NFIT_OLD_SET_CONFIG_DATA] = {
		.in_num = 4,
		.in_sizes = { 4, 4, 4, UINT_MAX, },
		.out_num = 1,
		.out_sizes = { 4, },
	},
	[NFIT_OLD_VENDOR] = {
		.in_num = 3,
		.in_sizes = { 4, 4, UINT_MAX, },
		.out_num = 3,
		.out_sizes = { 4, 4, UINT_MAX, },
	},
	[NFIT_OLD_ARS_CAP] = {
		.in_num = 2,
		.in_sizes = { 8, 8, },
		.out_num = 1,
		.out_sizes = { 4, },
	},
	[NFIT_OLD_ARS_START] = {
		.in_num = 3,
		.in_sizes = { 8, 8, 2, },
		.out_num = 1,
		.out_sizes = { 4, },
	},
	[NFIT_OLD_ARS_QUERY] = {
		.out_num = 2,
		.out_sizes = { 4, UINT_MAX, },
	},
	[NFIT_OLD_SMART_THRESHOLD] = {
		.in_num = 1,
		.in_sizes = { 4, },
		.out_num = 2,
		.out_sizes = { 4, 8, },
	},
};

static const struct cmd_desc *to_cmd_desc(unsigned int cmd)
{
	if (cmd <= NFIT_OLD_SMART_THRESHOLD && cmd >= NFIT_OLD_SMART)
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

	if (cmd == NFIT_OLD_SET_CONFIG_DATA && idx == 3) {
		struct nfit_cmd_set_config_hdr *hdr = buf + sizeof(u32);

		return hdr->in_length;
	} else if (cmd == NFIT_OLD_VENDOR && idx == 2) {
		struct nfit_cmd_vendor_hdr *hdr = buf + sizeof(u32);

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

	if (cmd == NFIT_OLD_GET_CONFIG_DATA && idx == 1) {
		struct nfit_cmd_get_config_data_hdr *hdr = buf + sizeof(u32);

		return hdr->in_length;
	} else if (cmd == NFIT_OLD_VENDOR && idx == 2
			&& offset < out_length) {
		return out_length - offset;
	} else if (cmd == NFIT_OLD_ARS_QUERY && idx == 1
			&& offset < out_length) {
		return out_length - offset;
	}

	return UINT_MAX;
}

static unsigned int to_old_cmd(unsigned int cmd, struct nd_dimm *nd_dimm)
{
	if (!nd_dimm) {
		switch (cmd) {
		case NFIT_CMD_ARS_CAP:
		case NFIT_CMD_ARS_START:
			return UINT_MAX; /* format incompat */
		case NFIT_CMD_ARS_QUERY: return NFIT_OLD_ARS_QUERY;
		default:
			return cmd;
		}
	}

	switch (cmd) {
	case NFIT_CMD_SMART: return NFIT_OLD_SMART;
	case NFIT_CMD_SMART_THRESHOLD: return NFIT_OLD_SMART_THRESHOLD;
	case NFIT_CMD_GET_CONFIG_SIZE: return NFIT_OLD_GET_CONFIG_SIZE;
	case NFIT_CMD_GET_CONFIG_DATA: return NFIT_OLD_GET_CONFIG_DATA;
	case NFIT_CMD_SET_CONFIG_DATA: return NFIT_OLD_SET_CONFIG_DATA;
	case NFIT_CMD_VENDOR: return NFIT_OLD_VENDOR;
	default:
		return cmd;
	}
}

static u8 nd_old_acpi_uuid[16]; /* initialized at nd_old_acpi_init */

static int nd_old_acpi_ctl(struct nfit_bus_descriptor *nfit_desc,
		struct nd_dimm *nd_dimm, unsigned int cmd, void *__buf,
		unsigned int buf_len)
{
	struct acpi_nfit *nfit = to_acpi_nfit(nfit_desc);
	union acpi_object in_obj, in_buf, *out_obj;
	acpi_handle handle = nfit->dev->handle;
	struct device *dev = &nfit->dev->dev;
	const struct cmd_desc *desc = NULL;
	int rc, i, old_nfit_offset;
	unsigned long dsm_mask;
	const char *cmd_name;
	unsigned int old_cmd;
	u32 nfit_handle;
	u32 offset;
	void *buf;

	if (test_bit(cmd, &nd_manual_dsm))
		return nd_dsm_ctl(nfit_desc, cmd, __buf, buf_len);

	if (nd_dimm)
		cmd_name = nfit_dimm_cmd_name(cmd);
	else
		cmd_name = nfit_bus_cmd_name(cmd);

	dsm_mask = nd_dimm ? nd_dimm_get_dsm_mask(nd_dimm) : nfit_desc->dsm_mask;
	if (!test_bit(cmd, &dsm_mask))
		return -ENOTTY;

	old_cmd = to_old_cmd(cmd, nd_dimm);
	if (old_cmd == UINT_MAX) {
		dev_err(dev, "%s legacy translation not supported\n", cmd_name);
		return -ENXIO;
	}
	cmd = old_cmd;
	desc = to_cmd_desc(cmd);

	if (!desc)
		return -ENOTTY;

	old_nfit_offset = nd_dimm ? sizeof(nfit_handle) : 0;
	buf_len += old_nfit_offset;
	buf = kmalloc(buf_len, GFP_KERNEL | __GFP_NOWARN);
	if (!buf)
		buf = vmalloc(buf_len);
	if (!buf)
		return -ENOMEM;
	memcpy(buf + old_nfit_offset, __buf, buf_len - old_nfit_offset);
	if (old_nfit_offset) {
		nfit_handle = to_nfit_handle(nd_dimm);
		memcpy(buf, &nfit_handle, sizeof(nfit_handle));
	}
	in_obj.type = ACPI_TYPE_PACKAGE;
	in_obj.package.count = 1;
	in_obj.package.elements = &in_buf;
	in_buf.type = ACPI_TYPE_BUFFER;
	in_buf.buffer.pointer = buf;
	in_buf.buffer.length = 0;

	/* double check that the nfit_acpi_cmd_descs table is self consistent */
	if (desc->in_num > NFIT_ACPI_MAX_ELEM) {
		WARN_ON_ONCE(1);
		rc = -ENXIO;
		goto err;
	}

	for (i = 0; i < desc->in_num; i++) {
		u32 in_size;

		in_size = to_cmd_in_size(cmd, desc, i, buf);
		if (in_size == UINT_MAX) {
			dev_err(dev, "%s: unknown input size cmd: %s field: %d\n",
					__func__, cmd_name, i);
			rc = -ENXIO;
			goto err;
		}
		in_buf.buffer.length += in_size;
		if (in_buf.buffer.length > buf_len) {
			dev_err(dev, "%s: input underrun cmd: %s field: %d\n",
					__func__, cmd_name, i);
			rc = -ENXIO;
			goto err;
		}
	}

	dev_dbg(dev, "%s: cmd: %s input length: %d\n", __func__,
			cmd_name, in_buf.buffer.length);
	if (IS_ENABLED(CONFIG_NFIT_ACPI_DEBUG))
		print_hex_dump_debug(cmd_name, DUMP_PREFIX_OFFSET, 4,
				4, in_buf.buffer.pointer, min_t(u32, 128,
					in_buf.buffer.length), true);

	out_obj = acpi_evaluate_dsm(handle, nd_old_acpi_uuid, 1, cmd, &in_obj);
	if (!out_obj) {
		dev_dbg(dev, "%s: _DSM failed cmd: %s\n", __func__,
				cmd_name);
		rc = -EINVAL;
		goto err;
	}

	if (out_obj->package.type != ACPI_TYPE_BUFFER) {
		dev_dbg(dev, "%s: unexpected output object type cmd: %s type: %d\n",
				__func__, cmd_name, out_obj->type);
		rc = -EINVAL;
		goto out;
	}
	dev_dbg(dev, "%s: cmd: %s output length: %d\n", __func__,
			cmd_name, out_obj->buffer.length);
	if (IS_ENABLED(CONFIG_NFIT_ACPI_DEBUG))
		print_hex_dump_debug(cmd_name, DUMP_PREFIX_OFFSET, 4,
				4, out_obj->buffer.pointer, min_t(u32, 128,
					out_obj->buffer.length), true);

	for (i = 0, offset = 0; i < desc->out_num; i++) {
		u32 out_size = to_cmd_out_size(cmd, desc, i, buf,
				out_obj->buffer.length, offset);

		if (out_size == UINT_MAX) {
			dev_dbg(dev, "%s: unknown output size cmd: %s field: %d\n",
					__func__, cmd_name, i);
			break;
		}

		if (offset + out_size > out_obj->buffer.length) {
			dev_dbg(dev, "%s: output object underflow cmd: %s field: %d\n",
					__func__, cmd_name, i);
			break;
		}

		if (in_buf.buffer.length + offset + out_size > buf_len) {
			dev_dbg(dev, "%s: output overrun cmd: %s field: %d\n",
					__func__, cmd_name, i);
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
					__func__, cmd_name, buf_len, offset);
			rc = -ENXIO;
		}
	} else
		rc = 0;
	memcpy(__buf, buf + old_nfit_offset, buf_len - old_nfit_offset);
 out:
	ACPI_FREE(out_obj);
 err:
	if (is_vmalloc_addr(buf))
		vfree(buf);
	else
		kfree(buf);

	return rc;
}

#define DIMM_DSM_MASK (\
	  1 << NFIT_CMD_SMART           \
	| 1 << NFIT_CMD_GET_CONFIG_SIZE \
	| 1 << NFIT_CMD_GET_CONFIG_DATA \
	| 1 << NFIT_CMD_SET_CONFIG_DATA \
	| 1 << NFIT_CMD_VENDOR_EFFECT_LOG_SIZE \
	| 1 << NFIT_CMD_VENDOR_EFFECT_LOG \
	| 1 << NFIT_CMD_VENDOR          \
	| 1 << NFIT_CMD_SMART_THRESHOLD)

static int nd_old_acpi_add_dimm(struct nfit_bus_descriptor *nfit_desc,
		struct nd_dimm *nd_dimm)
{
	unsigned long dsm_mask = 0;

	dsm_mask |= (nd_manual_dsm | force_dsm) & DIMM_DSM_MASK;
	nd_dimm_set_dsm_mask(nd_dimm, dsm_mask);
	return 0;
}

static acpi_status nd_old_acpi_add_nfit(struct acpi_resource *resource,
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
	nfit_desc->provider_name = "OLD_ACPI.NFIT";
	nfit_desc->nfit_ctl = nd_old_acpi_ctl;
	nfit_desc->add_dimm = nd_old_acpi_add_dimm;
	nfit_desc->dsm_mask |= (nd_manual_dsm | force_dsm) & ~DIMM_DSM_MASK;

	nfit->nd_bus = nfit_bus_register(&dev->dev, nfit_desc);
	if (!nfit->nd_bus)
		return AE_ERROR;
	else {
		dev_set_drvdata(&dev->dev, nfit);
		return AE_OK;
	}
}

static int nd_old_acpi_add(struct acpi_device *dev)
{
	acpi_status status;

	status = acpi_walk_resources(dev->handle, METHOD_NAME__CRS,
				     nd_old_acpi_add_nfit, dev);
	if (ACPI_FAILURE(status))
		return -EINVAL;

	dev_err(&dev->dev, "%s: discovering NFIT by _CRS is deprecated\n",
			__func__);
	add_taint(TAINT_FIRMWARE_WORKAROUND, LOCKDEP_STILL_OK);

	return 0;
}

static int nd_old_acpi_remove(struct acpi_device *dev)
{
	struct acpi_nfit *nfit = dev_get_drvdata(&dev->dev);
	struct nfit_bus_descriptor *nfit_desc = &nfit->nfit_desc;

	nfit_bus_unregister(nfit->nd_bus);
	iounmap(nfit_desc->nfit_base);

	return 0;
}

static const struct acpi_device_id nd_old_acpi_ids[] = {
	{ "ACPI0010", 0 },
	{ "", 0 },
};
MODULE_DEVICE_TABLE(acpi, nd_old_acpi_ids);

static struct acpi_driver nd_old_acpi_driver = {
	.name = KBUILD_MODNAME,
	.ids = nd_old_acpi_ids,
	.ops = {
		.add = nd_old_acpi_add,
		.remove = nd_old_acpi_remove,
	},
};

static __init int nd_old_acpi_init(void)
{
	if (acpi_str_to_uuid("4309ac30-0d11-11e4-9191-0800200c9a66",
				nd_old_acpi_uuid) != AE_OK) {
		WARN_ON_ONCE(1);
		return -ENXIO;
	}

	return acpi_bus_register_driver(&nd_old_acpi_driver);
}

static __exit void nd_old_acpi_exit(void)
{
	acpi_bus_unregister_driver(&nd_old_acpi_driver);
}

module_init(nd_old_acpi_init);
module_exit(nd_old_acpi_exit);
MODULE_LICENSE("GPL v2");
MODULE_AUTHOR("Intel Corporation");
