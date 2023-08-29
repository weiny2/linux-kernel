// SPDX-License-Identifier: GPL-2.0-only
// Copyright(c) 2021 Intel Corporation. All rights reserved.

#include <linux/platform_device.h>
#include <linux/mod_devicetable.h>
#include <linux/module.h>
#include <linux/delay.h>
#include <linux/sizes.h>
#include <linux/bits.h>
#include <asm/unaligned.h>
#include <crypto/sha2.h>
#include <cxlmem.h>

#include "trace.h"

#define LSA_SIZE SZ_128K
#define FW_SIZE SZ_64M
#define FW_SLOTS 3
#define DEV_SIZE SZ_2G
#define EFFECT(x) (1U << x)
#define BASE_DYNAMIC_CAP_DPA DEV_SIZE

#define MOCK_INJECT_DEV_MAX 8
#define MOCK_INJECT_TEST_MAX 128

static unsigned int poison_inject_dev_max = MOCK_INJECT_DEV_MAX;

enum cxl_command_effects {
	CONF_CHANGE_COLD_RESET = 0,
	CONF_CHANGE_IMMEDIATE,
	DATA_CHANGE_IMMEDIATE,
	POLICY_CHANGE_IMMEDIATE,
	LOG_CHANGE_IMMEDIATE,
	SECURITY_CHANGE_IMMEDIATE,
	BACKGROUND_OP,
	SECONDARY_MBOX_SUPPORTED,
};

#define CXL_CMD_EFFECT_NONE cpu_to_le16(0)

static struct cxl_cel_entry mock_cel[] = {
	{
		.opcode = cpu_to_le16(CXL_MBOX_OP_GET_SUPPORTED_LOGS),
		.effect = CXL_CMD_EFFECT_NONE,
	},
	{
		.opcode = cpu_to_le16(CXL_MBOX_OP_IDENTIFY),
		.effect = CXL_CMD_EFFECT_NONE,
	},
	{
		.opcode = cpu_to_le16(CXL_MBOX_OP_GET_LSA),
		.effect = CXL_CMD_EFFECT_NONE,
	},
	{
		.opcode = cpu_to_le16(CXL_MBOX_OP_GET_PARTITION_INFO),
		.effect = CXL_CMD_EFFECT_NONE,
	},
	{
		.opcode = cpu_to_le16(CXL_MBOX_OP_SET_LSA),
		.effect = cpu_to_le16(EFFECT(CONF_CHANGE_IMMEDIATE) |
				      EFFECT(DATA_CHANGE_IMMEDIATE)),
	},
	{
		.opcode = cpu_to_le16(CXL_MBOX_OP_GET_HEALTH_INFO),
		.effect = CXL_CMD_EFFECT_NONE,
	},
	{
		.opcode = cpu_to_le16(CXL_MBOX_OP_GET_POISON),
		.effect = CXL_CMD_EFFECT_NONE,
	},
	{
		.opcode = cpu_to_le16(CXL_MBOX_OP_INJECT_POISON),
		.effect = cpu_to_le16(EFFECT(DATA_CHANGE_IMMEDIATE)),
	},
	{
		.opcode = cpu_to_le16(CXL_MBOX_OP_CLEAR_POISON),
		.effect = cpu_to_le16(EFFECT(DATA_CHANGE_IMMEDIATE)),
	},
	{
		.opcode = cpu_to_le16(CXL_MBOX_OP_GET_FW_INFO),
		.effect = CXL_CMD_EFFECT_NONE,
	},
	{
		.opcode = cpu_to_le16(CXL_MBOX_OP_TRANSFER_FW),
		.effect = cpu_to_le16(EFFECT(CONF_CHANGE_COLD_RESET) |
				      EFFECT(BACKGROUND_OP)),
	},
	{
		.opcode = cpu_to_le16(CXL_MBOX_OP_ACTIVATE_FW),
		.effect = cpu_to_le16(EFFECT(CONF_CHANGE_COLD_RESET) |
				      EFFECT(CONF_CHANGE_IMMEDIATE)),
	},
	{
		.opcode = cpu_to_le16(CXL_MBOX_OP_GET_DC_CONFIG),
		.effect = CXL_CMD_EFFECT_NONE,
	},
	{
		.opcode = cpu_to_le16(CXL_MBOX_OP_GET_DC_EXTENT_LIST),
		.effect = CXL_CMD_EFFECT_NONE,
	},
	{
		.opcode = cpu_to_le16(CXL_MBOX_OP_ADD_DC_RESPONSE),
		.effect = cpu_to_le16(EFFECT(CONF_CHANGE_IMMEDIATE)),
	},
	{
		.opcode = cpu_to_le16(CXL_MBOX_OP_RELEASE_DC),
		.effect = cpu_to_le16(EFFECT(CONF_CHANGE_IMMEDIATE)),
	},
};

/* See CXL 2.0 Table 181 Get Health Info Output Payload */
struct cxl_mbox_health_info {
	u8 health_status;
	u8 media_status;
	u8 ext_status;
	u8 life_used;
	__le16 temperature;
	__le32 dirty_shutdowns;
	__le32 volatile_errors;
	__le32 pmem_errors;
} __packed;

static struct {
	struct cxl_mbox_get_supported_logs gsl;
	struct cxl_gsl_entry entry;
} mock_gsl_payload = {
	.gsl = {
		.entries = cpu_to_le16(1),
	},
	.entry = {
		.uuid = DEFINE_CXL_CEL_UUID,
		.size = cpu_to_le32(sizeof(mock_cel)),
	},
};

#define PASS_TRY_LIMIT 3

#define CXL_TEST_EVENT_CNT_MAX 17

/* Set a number of events to return at a time for simulation.  */
#define CXL_TEST_EVENT_CNT 3

/*
 * @next_handle: next handle (index) to be stored to
 * @cur_handle: current handle (index) to be returned to the user on get_event
 * @nr_events: total events in this log
 * @nr_overflow: number of events added past the log size
 * @lock: protect these state variables
 * @events: array of pending events to be returned.
 */
struct mock_event_log {
	u16 next_handle;
	u16 cur_handle;
	u16 nr_events;
	u16 nr_overflow;
	rwlock_t lock;
	/* 1 extra slot to accommodate that handles can't be 0 */
	struct cxl_event_record_raw *events[CXL_TEST_EVENT_CNT_MAX+1];
};

struct mock_event_store {
	struct cxl_memdev_state *mds;
	struct mock_event_log mock_logs[CXL_EVENT_TYPE_MAX];
	u32 ev_status;
};

#define NUM_MOCK_DC_REGIONS 2
struct cxl_mockmem_data {
	void *lsa;
	void *fw;
	int fw_slot;
	int fw_staged;
	size_t fw_size;
	u32 security_state;
	u8 user_pass[NVDIMM_PASSPHRASE_LEN];
	u8 master_pass[NVDIMM_PASSPHRASE_LEN];
	int user_limit;
	int master_limit;
	struct mock_event_store mes;
	u8 event_buf[SZ_4K];
	u64 timestamp;
	struct cxl_dc_region_config dc_regions[NUM_MOCK_DC_REGIONS];
	u32 dc_ext_generation;
	struct xarray dc_extents;
	struct xarray dc_accepted_exts;
};

static struct mock_event_log *event_find_log(struct device *dev, int log_type)
{
	struct cxl_mockmem_data *mdata = dev_get_drvdata(dev);

	if (log_type >= CXL_EVENT_TYPE_MAX)
		return NULL;
	return &mdata->mes.mock_logs[log_type];
}

/* Handle can never be 0 use 1 based indexing for handle */
static void event_inc_handle(u16 *handle)
{
	*handle = (*handle + 1) % CXL_TEST_EVENT_CNT_MAX;
	if (!*handle)
		*handle = *handle + 1;
}

/* Add the event or free it on 'overflow' */
static void mes_add_event(struct mock_event_store *mes,
			  enum cxl_event_log_type log_type,
			  struct cxl_event_record_raw *event)
{
	struct device *dev = mes->mds->cxlds.dev;
	struct mock_event_log *log;
	u16 handle;

	if (WARN_ON(log_type >= CXL_EVENT_TYPE_MAX))
		return;

	log = &mes->mock_logs[log_type];

	write_lock(&log->lock);

	handle = log->next_handle;
	if ((handle + 1) == log->cur_handle) {
		log->nr_overflow++;
		dev_dbg(dev, "Overflowing %d\n", log_type);
		devm_kfree(dev, event);
		goto unlock;
	}

	dev_dbg(dev, "Log %d; handle %u\n", log_type, handle);
	event->hdr.handle = cpu_to_le16(handle);
	log->events[handle] = event;
	event_inc_handle(&log->next_handle);
	log->nr_events++;

unlock:
	write_unlock(&log->lock);
}

static void mes_del_event(struct device *dev,
			  struct mock_event_log *log,
			  u16 handle)
{
	struct cxl_event_record_raw *cur;

	lockdep_assert(lockdep_is_held(&log->lock));

	dev_dbg(dev, "Clearing event %u; cur %u\n", handle, log->cur_handle);
	cur = log->events[handle];
	if (!cur) {
		dev_err(dev, "Mock event index %u empty? nr_events %u",
			handle, log->nr_events);
		return;
	}
	log->events[handle] = NULL;

	event_inc_handle(&log->cur_handle);
	log->nr_events--;
	devm_kfree(dev, cur);
}

static int mock_get_event(struct device *dev, struct cxl_mbox_cmd *cmd)
{
	struct cxl_get_event_payload *pl;
	struct mock_event_log *log;
	u8 log_type;
	u16 handle;
	int i;

	if (cmd->size_in != sizeof(log_type))
		return -EINVAL;

	if (cmd->size_out < struct_size(pl, records, CXL_TEST_EVENT_CNT))
		return -EINVAL;

	log_type = *((u8 *)cmd->payload_in);
	if (log_type >= CXL_EVENT_TYPE_MAX)
		return -EINVAL;

	log = event_find_log(dev, log_type);
	if (!log)
		return 0;

	memset(cmd->payload_out, 0, cmd->size_out);
	pl = cmd->payload_out;

	read_lock(&log->lock);

	handle = log->cur_handle;
	dev_dbg(dev, "Get log %d handle %u next %u\n",
		log_type, handle, log->next_handle);
	for (i = 0;
	     i < CXL_TEST_EVENT_CNT && handle != log->next_handle;
	     i++, event_inc_handle(&handle)) {
		struct cxl_event_record_raw *cur;

		cur = log->events[handle];
		dev_dbg(dev, "Sending event log %d handle %d idx %u\n",
			log_type, le16_to_cpu(cur->hdr.handle), handle);
		memcpy(&pl->records[i], cur, sizeof(pl->records[i]));
	}

	pl->record_count = cpu_to_le16(i);
	if (log->nr_events > i)
		pl->flags |= CXL_GET_EVENT_FLAG_MORE_RECORDS;

	if (log->nr_overflow) {
		u64 ns;

		pl->flags |= CXL_GET_EVENT_FLAG_OVERFLOW;
		pl->overflow_err_count = cpu_to_le16(log->nr_overflow);
		ns = ktime_get_real_ns();
		ns -= 5000000000; /* 5s ago */
		pl->first_overflow_timestamp = cpu_to_le64(ns);
		ns = ktime_get_real_ns();
		ns -= 1000000000; /* 1s ago */
		pl->last_overflow_timestamp = cpu_to_le64(ns);
	}

	read_unlock(&log->lock);
	return 0;
}

static int mock_clear_event(struct device *dev, struct cxl_mbox_cmd *cmd)
{
	struct cxl_mbox_clear_event_payload *pl = cmd->payload_in;
	u8 log_type = pl->event_log;
	struct mock_event_log *log;
	int nr, rc = 0;
	u16 handle;

	if (log_type >= CXL_EVENT_TYPE_MAX)
		return -EINVAL;

	log = event_find_log(dev, log_type);
	if (!log)
		return 0; /* No mock data in this log */

	write_lock(&log->lock);

	/* Check handle order prior to clearing events */
	handle = log->cur_handle;
	for (nr = 0;
	     nr < pl->nr_recs && handle != log->next_handle;
	     nr++, event_inc_handle(&handle)) {

		dev_dbg(dev, "Checking clear of %d handle %u plhandle %u\n",
			log_type, handle,
			le16_to_cpu(pl->handles[nr]));

		if (handle != le16_to_cpu(pl->handles[nr])) {
			dev_err(dev, "Clearing events out of order %u %u\n",
				handle, le16_to_cpu(pl->handles[nr]));
			rc = -EINVAL;
			goto unlock;
		}
	}

	if (log->nr_overflow)
		log->nr_overflow = 0;

	/* Clear events */
	for (nr = 0; nr < pl->nr_recs; nr++)
		mes_del_event(dev, log, le16_to_cpu(pl->handles[nr]));

unlock:
	write_unlock(&log->lock);
	return rc;
}

struct cxl_event_record_raw maint_needed = {
	.hdr = {
		.id = UUID_INIT(0xBA5EBA11, 0xABCD, 0xEFEB,
				0xa5, 0x5a, 0xa5, 0x5a, 0xa5, 0xa5, 0x5a, 0xa5),
		.length = sizeof(struct cxl_event_record_raw),
		.flags[0] = CXL_EVENT_RECORD_FLAG_MAINT_NEEDED,
		/* .handle = Set dynamically */
		.related_handle = cpu_to_le16(0xa5b6),
	},
	.data = { 0xDE, 0xAD, 0xBE, 0xEF },
};

struct cxl_event_record_raw hardware_replace = {
	.hdr = {
		.id = UUID_INIT(0xABCDEFEB, 0xBA11, 0xBA5E,
				0xa5, 0x5a, 0xa5, 0x5a, 0xa5, 0xa5, 0x5a, 0xa5),
		.length = sizeof(struct cxl_event_record_raw),
		.flags[0] = CXL_EVENT_RECORD_FLAG_HW_REPLACE,
		/* .handle = Set dynamically */
		.related_handle = cpu_to_le16(0xb6a5),
	},
	.data = { 0xDE, 0xAD, 0xBE, 0xEF },
};

struct cxl_event_gen_media gen_media = {
	.hdr = {
		.id = UUID_INIT(0xfbcd0a77, 0xc260, 0x417f,
				0x85, 0xa9, 0x08, 0x8b, 0x16, 0x21, 0xeb, 0xa6),
		.length = sizeof(struct cxl_event_gen_media),
		.flags[0] = CXL_EVENT_RECORD_FLAG_PERMANENT,
		/* .handle = Set dynamically */
		.related_handle = cpu_to_le16(0),
	},
	.phys_addr = cpu_to_le64(0x2000),
	.descriptor = CXL_GMER_EVT_DESC_UNCORECTABLE_EVENT,
	.type = CXL_GMER_MEM_EVT_TYPE_DATA_PATH_ERROR,
	.transaction_type = CXL_GMER_TRANS_HOST_WRITE,
	/* .validity_flags = <set below> */
	.channel = 1,
	.rank = 30
};

struct cxl_event_dram dram = {
	.hdr = {
		.id = UUID_INIT(0x601dcbb3, 0x9c06, 0x4eab,
				0xb8, 0xaf, 0x4e, 0x9b, 0xfb, 0x5c, 0x96, 0x24),
		.length = sizeof(struct cxl_event_dram),
		.flags[0] = CXL_EVENT_RECORD_FLAG_PERF_DEGRADED,
		/* .handle = Set dynamically */
		.related_handle = cpu_to_le16(0),
	},
	.phys_addr = cpu_to_le64(0x8000),
	.descriptor = CXL_GMER_EVT_DESC_THRESHOLD_EVENT,
	.type = CXL_GMER_MEM_EVT_TYPE_INV_ADDR,
	.transaction_type = CXL_GMER_TRANS_INTERNAL_MEDIA_SCRUB,
	/* .validity_flags = <set below> */
	.channel = 1,
	.bank_group = 5,
	.bank = 2,
	.column = {0xDE, 0xAD},
};

struct cxl_event_mem_module mem_module = {
	.hdr = {
		.id = UUID_INIT(0xfe927475, 0xdd59, 0x4339,
				0xa5, 0x86, 0x79, 0xba, 0xb1, 0x13, 0xb7, 0x74),
		.length = sizeof(struct cxl_event_mem_module),
		/* .handle = Set dynamically */
		.related_handle = cpu_to_le16(0),
	},
	.event_type = CXL_MMER_TEMP_CHANGE,
	.info = {
		.health_status = CXL_DHI_HS_PERFORMANCE_DEGRADED,
		.media_status = CXL_DHI_MS_ALL_DATA_LOST,
		.add_status = (CXL_DHI_AS_CRITICAL << 2) |
			      (CXL_DHI_AS_WARNING << 4) |
			      (CXL_DHI_AS_WARNING << 5),
		.device_temp = { 0xDE, 0xAD},
		.dirty_shutdown_cnt = { 0xde, 0xad, 0xbe, 0xef },
		.cor_vol_err_cnt = { 0xde, 0xad, 0xbe, 0xef },
		.cor_per_err_cnt = { 0xde, 0xad, 0xbe, 0xef },
	}
};

static int mock_set_timestamp(struct cxl_dev_state *cxlds,
			      struct cxl_mbox_cmd *cmd)
{
	struct cxl_mockmem_data *mdata = dev_get_drvdata(cxlds->dev);
	struct cxl_mbox_set_timestamp_in *ts = cmd->payload_in;

	if (cmd->size_in != sizeof(*ts))
		return -EINVAL;

	if (cmd->size_out != 0)
		return -EINVAL;

	mdata->timestamp = le64_to_cpu(ts->timestamp);
	return 0;
}

/* Create a dynamically allocated event out of a statically defined event. */
static void add_event_from_static(struct mock_event_store *mes,
				  enum cxl_event_log_type log_type,
				  struct cxl_event_record_raw *raw)
{
	struct device *dev = mes->mds->cxlds.dev;
	struct cxl_event_record_raw *rec;

	rec = devm_kzalloc(dev, sizeof(*rec), GFP_KERNEL);
	if (!rec) {
		dev_err(dev, "Failed to alloc event for log\n");
		return;
	}

	memcpy(rec, raw, sizeof(*rec));
	mes_add_event(mes, log_type, rec);
}

static void cxl_mock_add_event_logs(struct cxl_mockmem_data *mdata)
{
	struct mock_event_store *mes = &mdata->mes;
	struct device *dev = mes->mds->cxlds.dev;

	put_unaligned_le16(CXL_GMER_VALID_CHANNEL | CXL_GMER_VALID_RANK,
			   &gen_media.validity_flags);

	put_unaligned_le16(CXL_DER_VALID_CHANNEL | CXL_DER_VALID_BANK_GROUP |
			   CXL_DER_VALID_BANK | CXL_DER_VALID_COLUMN,
			   &dram.validity_flags);

	dev_dbg(dev, "Generating fake event logs %d\n",
		CXL_EVENT_TYPE_INFO);
	add_event_from_static(mes, CXL_EVENT_TYPE_INFO, &maint_needed);
	add_event_from_static(mes, CXL_EVENT_TYPE_INFO,
		      (struct cxl_event_record_raw *)&gen_media);
	add_event_from_static(mes, CXL_EVENT_TYPE_INFO,
		      (struct cxl_event_record_raw *)&mem_module);
	mes->ev_status |= CXLDEV_EVENT_STATUS_INFO;

	dev_dbg(dev, "Generating fake event logs %d\n",
		CXL_EVENT_TYPE_FAIL);
	add_event_from_static(mes, CXL_EVENT_TYPE_FAIL, &maint_needed);
	add_event_from_static(mes, CXL_EVENT_TYPE_FAIL,
		      (struct cxl_event_record_raw *)&mem_module);
	add_event_from_static(mes, CXL_EVENT_TYPE_FAIL, &hardware_replace);
	add_event_from_static(mes, CXL_EVENT_TYPE_FAIL,
		      (struct cxl_event_record_raw *)&dram);
	add_event_from_static(mes, CXL_EVENT_TYPE_FAIL,
		      (struct cxl_event_record_raw *)&gen_media);
	add_event_from_static(mes, CXL_EVENT_TYPE_FAIL,
		      (struct cxl_event_record_raw *)&mem_module);
	add_event_from_static(mes, CXL_EVENT_TYPE_FAIL, &hardware_replace);
	add_event_from_static(mes, CXL_EVENT_TYPE_FAIL,
		      (struct cxl_event_record_raw *)&dram);
	/* Overflow this log */
	add_event_from_static(mes, CXL_EVENT_TYPE_FAIL, &hardware_replace);
	add_event_from_static(mes, CXL_EVENT_TYPE_FAIL, &hardware_replace);
	add_event_from_static(mes, CXL_EVENT_TYPE_FAIL, &hardware_replace);
	add_event_from_static(mes, CXL_EVENT_TYPE_FAIL, &hardware_replace);
	add_event_from_static(mes, CXL_EVENT_TYPE_FAIL, &hardware_replace);
	add_event_from_static(mes, CXL_EVENT_TYPE_FAIL, &hardware_replace);
	add_event_from_static(mes, CXL_EVENT_TYPE_FAIL, &hardware_replace);
	add_event_from_static(mes, CXL_EVENT_TYPE_FAIL, &hardware_replace);
	add_event_from_static(mes, CXL_EVENT_TYPE_FAIL, &hardware_replace);
	add_event_from_static(mes, CXL_EVENT_TYPE_FAIL, &hardware_replace);
	mes->ev_status |= CXLDEV_EVENT_STATUS_FAIL;

	dev_dbg(dev, "Generating fake event logs %d\n",
		CXL_EVENT_TYPE_FATAL);
	add_event_from_static(mes, CXL_EVENT_TYPE_FATAL, &hardware_replace);
	add_event_from_static(mes, CXL_EVENT_TYPE_FATAL,
		      (struct cxl_event_record_raw *)&dram);
	mes->ev_status |= CXLDEV_EVENT_STATUS_FATAL;
}

static void cxl_mock_event_trigger(struct device *dev)
{
	struct cxl_mockmem_data *mdata = dev_get_drvdata(dev);
	struct mock_event_store *mes = &mdata->mes;

	cxl_mock_add_event_logs(mdata);
	cxl_mem_get_event_records(mes->mds, mes->ev_status);
}

static int devm_add_extent(struct device *dev, u64 start, u64 length,
			   const char *tag)
{
	struct cxl_mockmem_data *mdata = dev_get_drvdata(dev);
	struct cxl_dc_extent_data *extent;

	extent = devm_kzalloc(dev, sizeof(*extent), GFP_KERNEL);
	if (!extent) {
		dev_dbg(dev, "Failed to allocate extent\n");
		return -ENOMEM;
	}
	extent->dpa_start = start;
	extent->length = length;
	memcpy(extent->tag, tag, min(sizeof(extent->tag), strlen(tag)));

	if (xa_insert(&mdata->dc_extents, start, extent, GFP_KERNEL)) {
		devm_kfree(dev, extent);
		dev_err(dev, "Failed xarry insert %llx\n", start);
		return -EINVAL;
	}
	mdata->dc_ext_generation++;

	return 0;
}

static int dc_accept_extent(struct device *dev, u64 start)
{
	struct cxl_mockmem_data *mdata = dev_get_drvdata(dev);

	dev_dbg(dev, "Accepting extent 0x%llx\n", start);
	return xa_insert(&mdata->dc_accepted_exts, start, (void *)start,
			 GFP_KERNEL);
}

static void release_dc_ext(void *md)
{
	struct cxl_mockmem_data *mdata = md;

	xa_destroy(&mdata->dc_extents);
	xa_destroy(&mdata->dc_accepted_exts);
}

static int cxl_mock_dc_region_setup(struct device *dev)
{
#define DUMMY_EXT_OFFSET SZ_256M
#define DUMMY_EXT_LENGTH SZ_256M
	struct cxl_mockmem_data *mdata = dev_get_drvdata(dev);
	u64 base_dpa = BASE_DYNAMIC_CAP_DPA;
	u32 dsmad_handle = 0xFADE;
	u64 decode_length = SZ_2G;
	u64 block_size = SZ_512;
	/* For testing make this smaller than decode length */
	u64 length = SZ_1G;
	int rc;

	xa_init(&mdata->dc_extents);
	xa_init(&mdata->dc_accepted_exts);

	rc = devm_add_action_or_reset(dev, release_dc_ext, mdata);
	if (rc)
		return rc;

	for (int i = 0; i < NUM_MOCK_DC_REGIONS; i++) {
		struct cxl_dc_region_config *conf = &mdata->dc_regions[i];

		dev_dbg(dev, "Creating DC region DC%d DPA:%llx LEN:%llx\n",
			i, base_dpa, length);

		conf->region_base = cpu_to_le64(base_dpa);
		conf->region_decode_length = cpu_to_le64(decode_length /
						CXL_CAPACITY_MULTIPLIER);
		conf->region_length = cpu_to_le64(length);
		conf->region_block_size = cpu_to_le64(block_size);
		conf->region_dsmad_handle = cpu_to_le32(dsmad_handle);
		dsmad_handle++;

		/* Pretend we have some previous accepted extents */
		rc = devm_add_extent(dev, base_dpa + DUMMY_EXT_OFFSET,
				     DUMMY_EXT_LENGTH, "CXL-TEST");
		if (rc)
			return rc;

		rc = dc_accept_extent(dev, base_dpa + DUMMY_EXT_OFFSET);
		if (rc)
			return rc;

		base_dpa += decode_length;
	}

	return 0;
}

static int mock_gsl(struct cxl_mbox_cmd *cmd)
{
	if (cmd->size_out < sizeof(mock_gsl_payload))
		return -EINVAL;

	memcpy(cmd->payload_out, &mock_gsl_payload, sizeof(mock_gsl_payload));
	cmd->size_out = sizeof(mock_gsl_payload);

	return 0;
}

static int mock_get_log(struct cxl_memdev_state *mds, struct cxl_mbox_cmd *cmd)
{
	struct cxl_mbox_get_log *gl = cmd->payload_in;
	u32 offset = le32_to_cpu(gl->offset);
	u32 length = le32_to_cpu(gl->length);
	uuid_t uuid = DEFINE_CXL_CEL_UUID;
	void *data = &mock_cel;

	if (cmd->size_in < sizeof(*gl))
		return -EINVAL;
	if (length > mds->payload_size)
		return -EINVAL;
	if (offset + length > sizeof(mock_cel))
		return -EINVAL;
	if (!uuid_equal(&gl->uuid, &uuid))
		return -EINVAL;
	if (length > cmd->size_out)
		return -EINVAL;

	memcpy(cmd->payload_out, data + offset, length);

	return 0;
}

static int mock_rcd_id(struct cxl_mbox_cmd *cmd)
{
	struct cxl_mbox_identify id = {
		.fw_revision = { "mock fw v1 " },
		.total_capacity =
			cpu_to_le64(DEV_SIZE / CXL_CAPACITY_MULTIPLIER),
		.volatile_capacity =
			cpu_to_le64(DEV_SIZE / CXL_CAPACITY_MULTIPLIER),
	};

	if (cmd->size_out < sizeof(id))
		return -EINVAL;

	memcpy(cmd->payload_out, &id, sizeof(id));

	return 0;
}

static int mock_id(struct cxl_mbox_cmd *cmd)
{
	struct cxl_mbox_identify id = {
		.fw_revision = { "mock fw v1 " },
		.lsa_size = cpu_to_le32(LSA_SIZE),
		.partition_align =
			cpu_to_le64(SZ_256M / CXL_CAPACITY_MULTIPLIER),
		.total_capacity =
			cpu_to_le64(DEV_SIZE / CXL_CAPACITY_MULTIPLIER),
		.inject_poison_limit = cpu_to_le16(MOCK_INJECT_TEST_MAX),
	};

	put_unaligned_le24(CXL_POISON_LIST_MAX, id.poison_list_max_mer);

	if (cmd->size_out < sizeof(id))
		return -EINVAL;

	memcpy(cmd->payload_out, &id, sizeof(id));

	return 0;
}

static int mock_partition_info(struct cxl_mbox_cmd *cmd)
{
	struct cxl_mbox_get_partition_info pi = {
		.active_volatile_cap =
			cpu_to_le64(DEV_SIZE / 2 / CXL_CAPACITY_MULTIPLIER),
		.active_persistent_cap =
			cpu_to_le64(DEV_SIZE / 2 / CXL_CAPACITY_MULTIPLIER),
	};

	if (cmd->size_out < sizeof(pi))
		return -EINVAL;

	memcpy(cmd->payload_out, &pi, sizeof(pi));

	return 0;
}

static int mock_sanitize(struct cxl_mockmem_data *mdata,
			 struct cxl_mbox_cmd *cmd)
{
	if (cmd->size_in != 0)
		return -EINVAL;

	if (cmd->size_out != 0)
		return -EINVAL;

	if (mdata->security_state & CXL_PMEM_SEC_STATE_USER_PASS_SET) {
		cmd->return_code = CXL_MBOX_CMD_RC_SECURITY;
		return -ENXIO;
	}
	if (mdata->security_state & CXL_PMEM_SEC_STATE_LOCKED) {
		cmd->return_code = CXL_MBOX_CMD_RC_SECURITY;
		return -ENXIO;
	}

	return 0; /* assume less than 2 secs, no bg */
}

static int mock_secure_erase(struct cxl_mockmem_data *mdata,
			     struct cxl_mbox_cmd *cmd)
{
	if (cmd->size_in != 0)
		return -EINVAL;

	if (cmd->size_out != 0)
		return -EINVAL;

	if (mdata->security_state & CXL_PMEM_SEC_STATE_USER_PASS_SET) {
		cmd->return_code = CXL_MBOX_CMD_RC_SECURITY;
		return -ENXIO;
	}

	if (mdata->security_state & CXL_PMEM_SEC_STATE_LOCKED) {
		cmd->return_code = CXL_MBOX_CMD_RC_SECURITY;
		return -ENXIO;
	}

	return 0;
}

static int mock_get_security_state(struct cxl_mockmem_data *mdata,
				   struct cxl_mbox_cmd *cmd)
{
	if (cmd->size_in)
		return -EINVAL;

	if (cmd->size_out != sizeof(u32))
		return -EINVAL;

	memcpy(cmd->payload_out, &mdata->security_state, sizeof(u32));

	return 0;
}

static void master_plimit_check(struct cxl_mockmem_data *mdata)
{
	if (mdata->master_limit == PASS_TRY_LIMIT)
		return;
	mdata->master_limit++;
	if (mdata->master_limit == PASS_TRY_LIMIT)
		mdata->security_state |= CXL_PMEM_SEC_STATE_MASTER_PLIMIT;
}

static void user_plimit_check(struct cxl_mockmem_data *mdata)
{
	if (mdata->user_limit == PASS_TRY_LIMIT)
		return;
	mdata->user_limit++;
	if (mdata->user_limit == PASS_TRY_LIMIT)
		mdata->security_state |= CXL_PMEM_SEC_STATE_USER_PLIMIT;
}

static int mock_set_passphrase(struct cxl_mockmem_data *mdata,
			       struct cxl_mbox_cmd *cmd)
{
	struct cxl_set_pass *set_pass;

	if (cmd->size_in != sizeof(*set_pass))
		return -EINVAL;

	if (cmd->size_out != 0)
		return -EINVAL;

	if (mdata->security_state & CXL_PMEM_SEC_STATE_FROZEN) {
		cmd->return_code = CXL_MBOX_CMD_RC_SECURITY;
		return -ENXIO;
	}

	set_pass = cmd->payload_in;
	switch (set_pass->type) {
	case CXL_PMEM_SEC_PASS_MASTER:
		if (mdata->security_state & CXL_PMEM_SEC_STATE_MASTER_PLIMIT) {
			cmd->return_code = CXL_MBOX_CMD_RC_SECURITY;
			return -ENXIO;
		}
		/*
		 * CXL spec rev3.0 8.2.9.8.6.2, The master pasphrase shall only be set in
		 * the security disabled state when the user passphrase is not set.
		 */
		if (mdata->security_state & CXL_PMEM_SEC_STATE_USER_PASS_SET) {
			cmd->return_code = CXL_MBOX_CMD_RC_SECURITY;
			return -ENXIO;
		}
		if (memcmp(mdata->master_pass, set_pass->old_pass, NVDIMM_PASSPHRASE_LEN)) {
			master_plimit_check(mdata);
			cmd->return_code = CXL_MBOX_CMD_RC_PASSPHRASE;
			return -ENXIO;
		}
		memcpy(mdata->master_pass, set_pass->new_pass, NVDIMM_PASSPHRASE_LEN);
		mdata->security_state |= CXL_PMEM_SEC_STATE_MASTER_PASS_SET;
		return 0;

	case CXL_PMEM_SEC_PASS_USER:
		if (mdata->security_state & CXL_PMEM_SEC_STATE_USER_PLIMIT) {
			cmd->return_code = CXL_MBOX_CMD_RC_SECURITY;
			return -ENXIO;
		}
		if (memcmp(mdata->user_pass, set_pass->old_pass, NVDIMM_PASSPHRASE_LEN)) {
			user_plimit_check(mdata);
			cmd->return_code = CXL_MBOX_CMD_RC_PASSPHRASE;
			return -ENXIO;
		}
		memcpy(mdata->user_pass, set_pass->new_pass, NVDIMM_PASSPHRASE_LEN);
		mdata->security_state |= CXL_PMEM_SEC_STATE_USER_PASS_SET;
		return 0;

	default:
		cmd->return_code = CXL_MBOX_CMD_RC_INPUT;
	}
	return -EINVAL;
}

static int mock_disable_passphrase(struct cxl_mockmem_data *mdata,
				   struct cxl_mbox_cmd *cmd)
{
	struct cxl_disable_pass *dis_pass;

	if (cmd->size_in != sizeof(*dis_pass))
		return -EINVAL;

	if (cmd->size_out != 0)
		return -EINVAL;

	if (mdata->security_state & CXL_PMEM_SEC_STATE_FROZEN) {
		cmd->return_code = CXL_MBOX_CMD_RC_SECURITY;
		return -ENXIO;
	}

	dis_pass = cmd->payload_in;
	switch (dis_pass->type) {
	case CXL_PMEM_SEC_PASS_MASTER:
		if (mdata->security_state & CXL_PMEM_SEC_STATE_MASTER_PLIMIT) {
			cmd->return_code = CXL_MBOX_CMD_RC_SECURITY;
			return -ENXIO;
		}

		if (!(mdata->security_state & CXL_PMEM_SEC_STATE_MASTER_PASS_SET)) {
			cmd->return_code = CXL_MBOX_CMD_RC_SECURITY;
			return -ENXIO;
		}

		if (memcmp(dis_pass->pass, mdata->master_pass, NVDIMM_PASSPHRASE_LEN)) {
			master_plimit_check(mdata);
			cmd->return_code = CXL_MBOX_CMD_RC_PASSPHRASE;
			return -ENXIO;
		}

		mdata->master_limit = 0;
		memset(mdata->master_pass, 0, NVDIMM_PASSPHRASE_LEN);
		mdata->security_state &= ~CXL_PMEM_SEC_STATE_MASTER_PASS_SET;
		return 0;

	case CXL_PMEM_SEC_PASS_USER:
		if (mdata->security_state & CXL_PMEM_SEC_STATE_USER_PLIMIT) {
			cmd->return_code = CXL_MBOX_CMD_RC_SECURITY;
			return -ENXIO;
		}

		if (!(mdata->security_state & CXL_PMEM_SEC_STATE_USER_PASS_SET)) {
			cmd->return_code = CXL_MBOX_CMD_RC_SECURITY;
			return -ENXIO;
		}

		if (memcmp(dis_pass->pass, mdata->user_pass, NVDIMM_PASSPHRASE_LEN)) {
			user_plimit_check(mdata);
			cmd->return_code = CXL_MBOX_CMD_RC_PASSPHRASE;
			return -ENXIO;
		}

		mdata->user_limit = 0;
		memset(mdata->user_pass, 0, NVDIMM_PASSPHRASE_LEN);
		mdata->security_state &= ~(CXL_PMEM_SEC_STATE_USER_PASS_SET |
					   CXL_PMEM_SEC_STATE_LOCKED);
		return 0;

	default:
		cmd->return_code = CXL_MBOX_CMD_RC_INPUT;
		return -EINVAL;
	}

	return 0;
}

static int mock_freeze_security(struct cxl_mockmem_data *mdata,
				struct cxl_mbox_cmd *cmd)
{
	if (cmd->size_in != 0)
		return -EINVAL;

	if (cmd->size_out != 0)
		return -EINVAL;

	if (mdata->security_state & CXL_PMEM_SEC_STATE_FROZEN)
		return 0;

	mdata->security_state |= CXL_PMEM_SEC_STATE_FROZEN;
	return 0;
}

static int mock_unlock_security(struct cxl_mockmem_data *mdata,
				struct cxl_mbox_cmd *cmd)
{
	if (cmd->size_in != NVDIMM_PASSPHRASE_LEN)
		return -EINVAL;

	if (cmd->size_out != 0)
		return -EINVAL;

	if (mdata->security_state & CXL_PMEM_SEC_STATE_FROZEN) {
		cmd->return_code = CXL_MBOX_CMD_RC_SECURITY;
		return -ENXIO;
	}

	if (!(mdata->security_state & CXL_PMEM_SEC_STATE_USER_PASS_SET)) {
		cmd->return_code = CXL_MBOX_CMD_RC_SECURITY;
		return -ENXIO;
	}

	if (mdata->security_state & CXL_PMEM_SEC_STATE_USER_PLIMIT) {
		cmd->return_code = CXL_MBOX_CMD_RC_SECURITY;
		return -ENXIO;
	}

	if (!(mdata->security_state & CXL_PMEM_SEC_STATE_LOCKED)) {
		cmd->return_code = CXL_MBOX_CMD_RC_SECURITY;
		return -ENXIO;
	}

	if (memcmp(cmd->payload_in, mdata->user_pass, NVDIMM_PASSPHRASE_LEN)) {
		if (++mdata->user_limit == PASS_TRY_LIMIT)
			mdata->security_state |= CXL_PMEM_SEC_STATE_USER_PLIMIT;
		cmd->return_code = CXL_MBOX_CMD_RC_PASSPHRASE;
		return -ENXIO;
	}

	mdata->user_limit = 0;
	mdata->security_state &= ~CXL_PMEM_SEC_STATE_LOCKED;
	return 0;
}

static int mock_passphrase_secure_erase(struct cxl_mockmem_data *mdata,
					struct cxl_mbox_cmd *cmd)
{
	struct cxl_pass_erase *erase;

	if (cmd->size_in != sizeof(*erase))
		return -EINVAL;

	if (cmd->size_out != 0)
		return -EINVAL;

	erase = cmd->payload_in;
	if (mdata->security_state & CXL_PMEM_SEC_STATE_FROZEN) {
		cmd->return_code = CXL_MBOX_CMD_RC_SECURITY;
		return -ENXIO;
	}

	if (mdata->security_state & CXL_PMEM_SEC_STATE_USER_PLIMIT &&
	    erase->type == CXL_PMEM_SEC_PASS_USER) {
		cmd->return_code = CXL_MBOX_CMD_RC_SECURITY;
		return -ENXIO;
	}

	if (mdata->security_state & CXL_PMEM_SEC_STATE_MASTER_PLIMIT &&
	    erase->type == CXL_PMEM_SEC_PASS_MASTER) {
		cmd->return_code = CXL_MBOX_CMD_RC_SECURITY;
		return -ENXIO;
	}

	switch (erase->type) {
	case CXL_PMEM_SEC_PASS_MASTER:
		/*
		 * The spec does not clearly define the behavior of the scenario
		 * where a master passphrase is passed in while the master
		 * passphrase is not set and user passphrase is not set. The
		 * code will take the assumption that it will behave the same
		 * as a CXL secure erase command without passphrase (0x4401).
		 */
		if (mdata->security_state & CXL_PMEM_SEC_STATE_MASTER_PASS_SET) {
			if (memcmp(mdata->master_pass, erase->pass,
				   NVDIMM_PASSPHRASE_LEN)) {
				master_plimit_check(mdata);
				cmd->return_code = CXL_MBOX_CMD_RC_PASSPHRASE;
				return -ENXIO;
			}
			mdata->master_limit = 0;
			mdata->user_limit = 0;
			mdata->security_state &= ~CXL_PMEM_SEC_STATE_USER_PASS_SET;
			memset(mdata->user_pass, 0, NVDIMM_PASSPHRASE_LEN);
			mdata->security_state &= ~CXL_PMEM_SEC_STATE_LOCKED;
		} else {
			/*
			 * CXL rev3 8.2.9.8.6.3 Disable Passphrase
			 * When master passphrase is disabled, the device shall
			 * return Invalid Input for the Passphrase Secure Erase
			 * command with master passphrase.
			 */
			return -EINVAL;
		}
		/* Scramble encryption keys so that data is effectively erased */
		break;
	case CXL_PMEM_SEC_PASS_USER:
		/*
		 * The spec does not clearly define the behavior of the scenario
		 * where a user passphrase is passed in while the user
		 * passphrase is not set. The code will take the assumption that
		 * it will behave the same as a CXL secure erase command without
		 * passphrase (0x4401).
		 */
		if (mdata->security_state & CXL_PMEM_SEC_STATE_USER_PASS_SET) {
			if (memcmp(mdata->user_pass, erase->pass,
				   NVDIMM_PASSPHRASE_LEN)) {
				user_plimit_check(mdata);
				cmd->return_code = CXL_MBOX_CMD_RC_PASSPHRASE;
				return -ENXIO;
			}
			mdata->user_limit = 0;
			mdata->security_state &= ~CXL_PMEM_SEC_STATE_USER_PASS_SET;
			memset(mdata->user_pass, 0, NVDIMM_PASSPHRASE_LEN);
		}

		/*
		 * CXL rev3 Table 8-118
		 * If user passphrase is not set or supported by device, current
		 * passphrase value is ignored. Will make the assumption that
		 * the operation will proceed as secure erase w/o passphrase
		 * since spec is not explicit.
		 */

		/* Scramble encryption keys so that data is effectively erased */
		break;
	default:
		return -EINVAL;
	}

	return 0;
}

static int mock_get_lsa(struct cxl_mockmem_data *mdata,
			struct cxl_mbox_cmd *cmd)
{
	struct cxl_mbox_get_lsa *get_lsa = cmd->payload_in;
	void *lsa = mdata->lsa;
	u32 offset, length;

	if (sizeof(*get_lsa) > cmd->size_in)
		return -EINVAL;
	offset = le32_to_cpu(get_lsa->offset);
	length = le32_to_cpu(get_lsa->length);
	if (offset + length > LSA_SIZE)
		return -EINVAL;
	if (length > cmd->size_out)
		return -EINVAL;

	memcpy(cmd->payload_out, lsa + offset, length);
	return 0;
}

static int mock_set_lsa(struct cxl_mockmem_data *mdata,
			struct cxl_mbox_cmd *cmd)
{
	struct cxl_mbox_set_lsa *set_lsa = cmd->payload_in;
	void *lsa = mdata->lsa;
	u32 offset, length;

	if (sizeof(*set_lsa) > cmd->size_in)
		return -EINVAL;
	offset = le32_to_cpu(set_lsa->offset);
	length = cmd->size_in - sizeof(*set_lsa);
	if (offset + length > LSA_SIZE)
		return -EINVAL;

	memcpy(lsa + offset, &set_lsa->data[0], length);
	return 0;
}

static int mock_health_info(struct cxl_mbox_cmd *cmd)
{
	struct cxl_mbox_health_info health_info = {
		/* set flags for maint needed, perf degraded, hw replacement */
		.health_status = 0x7,
		/* set media status to "All Data Lost" */
		.media_status = 0x3,
		/*
		 * set ext_status flags for:
		 *  ext_life_used: normal,
		 *  ext_temperature: critical,
		 *  ext_corrected_volatile: warning,
		 *  ext_corrected_persistent: normal,
		 */
		.ext_status = 0x18,
		.life_used = 15,
		.temperature = cpu_to_le16(25),
		.dirty_shutdowns = cpu_to_le32(10),
		.volatile_errors = cpu_to_le32(20),
		.pmem_errors = cpu_to_le32(30),
	};

	if (cmd->size_out < sizeof(health_info))
		return -EINVAL;

	memcpy(cmd->payload_out, &health_info, sizeof(health_info));
	return 0;
}

static struct mock_poison {
	struct cxl_dev_state *cxlds;
	u64 dpa;
} mock_poison_list[MOCK_INJECT_TEST_MAX];

static struct cxl_mbox_poison_out *
cxl_get_injected_po(struct cxl_dev_state *cxlds, u64 offset, u64 length)
{
	struct cxl_mbox_poison_out *po;
	int nr_records = 0;
	u64 dpa;

	po = kzalloc(struct_size(po, record, poison_inject_dev_max), GFP_KERNEL);
	if (!po)
		return NULL;

	for (int i = 0; i < MOCK_INJECT_TEST_MAX; i++) {
		if (mock_poison_list[i].cxlds != cxlds)
			continue;
		if (mock_poison_list[i].dpa < offset ||
		    mock_poison_list[i].dpa > offset + length - 1)
			continue;

		dpa = mock_poison_list[i].dpa + CXL_POISON_SOURCE_INJECTED;
		po->record[nr_records].address = cpu_to_le64(dpa);
		po->record[nr_records].length = cpu_to_le32(1);
		nr_records++;
		if (nr_records == poison_inject_dev_max)
			break;
	}

	/* Always return count, even when zero */
	po->count = cpu_to_le16(nr_records);

	return po;
}

static int mock_get_poison(struct cxl_dev_state *cxlds,
			   struct cxl_mbox_cmd *cmd)
{
	struct cxl_mbox_poison_in *pi = cmd->payload_in;
	struct cxl_mbox_poison_out *po;
	u64 offset = le64_to_cpu(pi->offset);
	u64 length = le64_to_cpu(pi->length);
	int nr_records;

	po = cxl_get_injected_po(cxlds, offset, length);
	if (!po)
		return -ENOMEM;
	nr_records = le16_to_cpu(po->count);
	memcpy(cmd->payload_out, po, struct_size(po, record, nr_records));
	cmd->size_out = struct_size(po, record, nr_records);
	kfree(po);

	return 0;
}

static bool mock_poison_dev_max_injected(struct cxl_dev_state *cxlds)
{
	int count = 0;

	for (int i = 0; i < MOCK_INJECT_TEST_MAX; i++) {
		if (mock_poison_list[i].cxlds == cxlds)
			count++;
	}
	return (count >= poison_inject_dev_max);
}

static bool mock_poison_add(struct cxl_dev_state *cxlds, u64 dpa)
{
	if (mock_poison_dev_max_injected(cxlds)) {
		dev_dbg(cxlds->dev,
			"Device poison injection limit has been reached: %d\n",
			MOCK_INJECT_DEV_MAX);
		return false;
	}

	for (int i = 0; i < MOCK_INJECT_TEST_MAX; i++) {
		if (!mock_poison_list[i].cxlds) {
			mock_poison_list[i].cxlds = cxlds;
			mock_poison_list[i].dpa = dpa;
			return true;
		}
	}
	dev_dbg(cxlds->dev,
		"Mock test poison injection limit has been reached: %d\n",
		MOCK_INJECT_TEST_MAX);

	return false;
}

static bool mock_poison_found(struct cxl_dev_state *cxlds, u64 dpa)
{
	for (int i = 0; i < MOCK_INJECT_TEST_MAX; i++) {
		if (mock_poison_list[i].cxlds == cxlds &&
		    mock_poison_list[i].dpa == dpa)
			return true;
	}
	return false;
}

static int mock_inject_poison(struct cxl_dev_state *cxlds,
			      struct cxl_mbox_cmd *cmd)
{
	struct cxl_mbox_inject_poison *pi = cmd->payload_in;
	u64 dpa = le64_to_cpu(pi->address);

	if (mock_poison_found(cxlds, dpa)) {
		/* Not an error to inject poison if already poisoned */
		dev_dbg(cxlds->dev, "DPA: 0x%llx already poisoned\n", dpa);
		return 0;
	}
	if (!mock_poison_add(cxlds, dpa))
		return -ENXIO;

	return 0;
}

static bool mock_poison_del(struct cxl_dev_state *cxlds, u64 dpa)
{
	for (int i = 0; i < MOCK_INJECT_TEST_MAX; i++) {
		if (mock_poison_list[i].cxlds == cxlds &&
		    mock_poison_list[i].dpa == dpa) {
			mock_poison_list[i].cxlds = NULL;
			return true;
		}
	}
	return false;
}

static int mock_clear_poison(struct cxl_dev_state *cxlds,
			     struct cxl_mbox_cmd *cmd)
{
	struct cxl_mbox_clear_poison *pi = cmd->payload_in;
	u64 dpa = le64_to_cpu(pi->address);

	/*
	 * A real CXL device will write pi->write_data to the address
	 * being cleared. In this mock, just delete this address from
	 * the mock poison list.
	 */
	if (!mock_poison_del(cxlds, dpa))
		dev_dbg(cxlds->dev, "DPA: 0x%llx not in poison list\n", dpa);

	return 0;
}

static bool mock_poison_list_empty(void)
{
	for (int i = 0; i < MOCK_INJECT_TEST_MAX; i++) {
		if (mock_poison_list[i].cxlds)
			return false;
	}
	return true;
}

static ssize_t poison_inject_max_show(struct device_driver *drv, char *buf)
{
	return sysfs_emit(buf, "%u\n", poison_inject_dev_max);
}

static ssize_t poison_inject_max_store(struct device_driver *drv,
				       const char *buf, size_t len)
{
	int val;

	if (kstrtoint(buf, 0, &val) < 0)
		return -EINVAL;

	if (!mock_poison_list_empty())
		return -EBUSY;

	if (val <= MOCK_INJECT_TEST_MAX)
		poison_inject_dev_max = val;
	else
		return -EINVAL;

	return len;
}

static DRIVER_ATTR_RW(poison_inject_max);

static struct attribute *cxl_mock_mem_core_attrs[] = {
	&driver_attr_poison_inject_max.attr,
	NULL
};
ATTRIBUTE_GROUPS(cxl_mock_mem_core);

static int mock_fw_info(struct cxl_mockmem_data *mdata,
			struct cxl_mbox_cmd *cmd)
{
	struct cxl_mbox_get_fw_info fw_info = {
		.num_slots = FW_SLOTS,
		.slot_info = (mdata->fw_slot & 0x7) |
			     ((mdata->fw_staged & 0x7) << 3),
		.activation_cap = 0,
	};

	strcpy(fw_info.slot_1_revision, "cxl_test_fw_001");
	strcpy(fw_info.slot_2_revision, "cxl_test_fw_002");
	strcpy(fw_info.slot_3_revision, "cxl_test_fw_003");
	strcpy(fw_info.slot_4_revision, "");

	if (cmd->size_out < sizeof(fw_info))
		return -EINVAL;

	memcpy(cmd->payload_out, &fw_info, sizeof(fw_info));
	return 0;
}

static int mock_transfer_fw(struct cxl_mockmem_data *mdata,
			    struct cxl_mbox_cmd *cmd)
{
	struct cxl_mbox_transfer_fw *transfer = cmd->payload_in;
	void *fw = mdata->fw;
	size_t offset, length;

	offset = le32_to_cpu(transfer->offset) * CXL_FW_TRANSFER_ALIGNMENT;
	length = cmd->size_in - sizeof(*transfer);
	if (offset + length > FW_SIZE)
		return -EINVAL;

	switch (transfer->action) {
	case CXL_FW_TRANSFER_ACTION_FULL:
		if (offset != 0)
			return -EINVAL;
		fallthrough;
	case CXL_FW_TRANSFER_ACTION_END:
		if (transfer->slot == 0 || transfer->slot > FW_SLOTS)
			return -EINVAL;
		mdata->fw_size = offset + length;
		break;
	case CXL_FW_TRANSFER_ACTION_INITIATE:
	case CXL_FW_TRANSFER_ACTION_CONTINUE:
		break;
	case CXL_FW_TRANSFER_ACTION_ABORT:
		return 0;
	default:
		return -EINVAL;
	}

	memcpy(fw + offset, transfer->data, length);
	return 0;
}

static int mock_activate_fw(struct cxl_mockmem_data *mdata,
			    struct cxl_mbox_cmd *cmd)
{
	struct cxl_mbox_activate_fw *activate = cmd->payload_in;

	if (activate->slot == 0 || activate->slot > FW_SLOTS)
		return -EINVAL;

	switch (activate->action) {
	case CXL_FW_ACTIVATE_ONLINE:
		mdata->fw_slot = activate->slot;
		mdata->fw_staged = 0;
		return 0;
	case CXL_FW_ACTIVATE_OFFLINE:
		mdata->fw_staged = activate->slot;
		return 0;
	}

	return -EINVAL;
}

static int mock_get_dc_config(struct device *dev,
			      struct cxl_mbox_cmd *cmd)
{
	struct cxl_mbox_get_dc_config *dc_config = cmd->payload_in;
	struct cxl_mockmem_data *mdata = dev_get_drvdata(dev);
	u8 region_requested, region_start_idx, region_ret_cnt;
	struct cxl_mbox_dynamic_capacity *resp;

	region_requested = dc_config->region_count;
	if (NUM_MOCK_DC_REGIONS < region_requested)
		region_requested = NUM_MOCK_DC_REGIONS;

	if (cmd->size_out < struct_size(resp, region, region_requested))
		return -EINVAL;

	memset(cmd->payload_out, 0, cmd->size_out);
	resp = cmd->payload_out;

	region_start_idx = dc_config->start_region_index;
	region_ret_cnt = 0;
	for (int i = 0; i < NUM_MOCK_DC_REGIONS; i++) {
		if (i >= region_start_idx) {
			memcpy(&resp->region[region_ret_cnt],
				&mdata->dc_regions[i],
				sizeof(resp->region[region_ret_cnt]));
			region_ret_cnt++;
		}
	}
	resp->avail_region_count = region_ret_cnt;

	dev_dbg(dev, "Returning %d dc regions\n", region_ret_cnt);
	return 0;
}


static int mock_get_dc_extent_list(struct device *dev,
				   struct cxl_mbox_cmd *cmd)
{
	struct cxl_mockmem_data *mdata = dev_get_drvdata(dev);
	struct cxl_mbox_get_dc_extent *get = cmd->payload_in;
	struct cxl_mbox_dc_extents *resp = cmd->payload_out;
	u32 total_avail = 0, total_ret = 0;
	struct cxl_dc_extent_data *ext;
	u32 ext_count, start_idx;
	unsigned long i;

	ext_count = le32_to_cpu(get->extent_cnt);
	start_idx = le32_to_cpu(get->start_extent_index);

	memset(resp, 0, sizeof(*resp));

	/*
	 * Total available needs to be calculated and returned regardless of
	 * how many can actually be returned.
	 */
	xa_for_each(&mdata->dc_extents, i, ext)
		total_avail++;

	if (start_idx > total_avail)
		return -EINVAL;

	xa_for_each(&mdata->dc_extents, i, ext) {
		if (total_ret >= ext_count)
			break;

		if (total_ret >= start_idx) {
			resp->extent[total_ret].start_dpa =
						cpu_to_le64(ext->dpa_start);
			resp->extent[total_ret].length =
						cpu_to_le64(ext->length);
			memcpy(&resp->extent[total_ret].tag, ext->tag,
					sizeof(resp->extent[total_ret]));
			resp->extent[total_ret].shared_extn_seq =
					cpu_to_le16(ext->shared_extent_seq);
			total_ret++;
		}
	}

	resp->ret_extent_cnt = cpu_to_le32(total_ret);
	resp->total_extent_cnt = cpu_to_le32(total_avail);
	resp->extent_list_num = cpu_to_le32(mdata->dc_ext_generation);

	dev_dbg(dev, "Returning %d extents of %d total\n",
		total_ret, total_avail);

	return 0;
}

static int mock_add_dc_response(struct device *dev,
				struct cxl_mbox_cmd *cmd)
{
	struct cxl_mbox_dc_response *req = cmd->payload_in;
	u32 list_size = le32_to_cpu(req->extent_list_size);

	for (int i = 0; i < list_size; i++) {
		u64 start = le64_to_cpu(req->extent_list[i].dpa_start);
		int rc;

		dev_dbg(dev, "Extent 0x%llx accepted by HOST\n", start);
		rc = dc_accept_extent(dev, start);
		if (rc)
			return rc;
	}

	return 0;
}

static int dc_delete_extent(struct device *dev, unsigned long long start)
{
	struct cxl_mockmem_data *mdata = dev_get_drvdata(dev);
	void *ext;

	dev_dbg(dev, "Deleting extent at %llx\n", start);

	ext = xa_erase(&mdata->dc_extents, start);
	if (!ext) {
		dev_err(dev, "No extent found at %llx\n", start);
		return -EINVAL;
	}
	devm_kfree(dev, ext);
	mdata->dc_ext_generation++;

	return 0;
}

static int mock_dc_release(struct device *dev,
			   struct cxl_mbox_cmd *cmd)
{
	struct cxl_mockmem_data *mdata = dev_get_drvdata(dev);
	struct cxl_mbox_dc_response *req = cmd->payload_in;
	u32 list_size = le32_to_cpu(req->extent_list_size);

	for (int i = 0; i < list_size; i++) {
		u64 start = le64_to_cpu(req->extent_list[i].dpa_start);

		dev_dbg(dev, "Extent 0x%llx released by HOST\n", start);
		xa_erase(&mdata->dc_accepted_exts, start);
	}

	return 0;
}

static int cxl_mock_mbox_send(struct cxl_memdev_state *mds,
			      struct cxl_mbox_cmd *cmd)
{
	struct cxl_dev_state *cxlds = &mds->cxlds;
	struct device *dev = cxlds->dev;
	struct cxl_mockmem_data *mdata = dev_get_drvdata(dev);
	int rc = -EIO;

	switch (cmd->opcode) {
	case CXL_MBOX_OP_SET_TIMESTAMP:
		rc = mock_set_timestamp(cxlds, cmd);
		break;
	case CXL_MBOX_OP_GET_SUPPORTED_LOGS:
		rc = mock_gsl(cmd);
		break;
	case CXL_MBOX_OP_GET_LOG:
		rc = mock_get_log(mds, cmd);
		break;
	case CXL_MBOX_OP_IDENTIFY:
		if (cxlds->rcd)
			rc = mock_rcd_id(cmd);
		else
			rc = mock_id(cmd);
		break;
	case CXL_MBOX_OP_GET_LSA:
		rc = mock_get_lsa(mdata, cmd);
		break;
	case CXL_MBOX_OP_GET_PARTITION_INFO:
		rc = mock_partition_info(cmd);
		break;
	case CXL_MBOX_OP_GET_EVENT_RECORD:
		rc = mock_get_event(dev, cmd);
		break;
	case CXL_MBOX_OP_CLEAR_EVENT_RECORD:
		rc = mock_clear_event(dev, cmd);
		break;
	case CXL_MBOX_OP_SET_LSA:
		rc = mock_set_lsa(mdata, cmd);
		break;
	case CXL_MBOX_OP_GET_HEALTH_INFO:
		rc = mock_health_info(cmd);
		break;
	case CXL_MBOX_OP_SANITIZE:
		rc = mock_sanitize(mdata, cmd);
		break;
	case CXL_MBOX_OP_SECURE_ERASE:
		rc = mock_secure_erase(mdata, cmd);
		break;
	case CXL_MBOX_OP_GET_SECURITY_STATE:
		rc = mock_get_security_state(mdata, cmd);
		break;
	case CXL_MBOX_OP_SET_PASSPHRASE:
		rc = mock_set_passphrase(mdata, cmd);
		break;
	case CXL_MBOX_OP_DISABLE_PASSPHRASE:
		rc = mock_disable_passphrase(mdata, cmd);
		break;
	case CXL_MBOX_OP_FREEZE_SECURITY:
		rc = mock_freeze_security(mdata, cmd);
		break;
	case CXL_MBOX_OP_UNLOCK:
		rc = mock_unlock_security(mdata, cmd);
		break;
	case CXL_MBOX_OP_PASSPHRASE_SECURE_ERASE:
		rc = mock_passphrase_secure_erase(mdata, cmd);
		break;
	case CXL_MBOX_OP_GET_POISON:
		rc = mock_get_poison(cxlds, cmd);
		break;
	case CXL_MBOX_OP_INJECT_POISON:
		rc = mock_inject_poison(cxlds, cmd);
		break;
	case CXL_MBOX_OP_CLEAR_POISON:
		rc = mock_clear_poison(cxlds, cmd);
		break;
	case CXL_MBOX_OP_GET_FW_INFO:
		rc = mock_fw_info(mdata, cmd);
		break;
	case CXL_MBOX_OP_TRANSFER_FW:
		rc = mock_transfer_fw(mdata, cmd);
		break;
	case CXL_MBOX_OP_ACTIVATE_FW:
		rc = mock_activate_fw(mdata, cmd);
		break;
	case CXL_MBOX_OP_GET_DC_CONFIG:
		rc = mock_get_dc_config(dev, cmd);
		break;
	case CXL_MBOX_OP_GET_DC_EXTENT_LIST:
		rc = mock_get_dc_extent_list(dev, cmd);
		break;
	case CXL_MBOX_OP_ADD_DC_RESPONSE:
		rc = mock_add_dc_response(dev, cmd);
		break;
	case CXL_MBOX_OP_RELEASE_DC:
		rc = mock_dc_release(dev, cmd);
		break;
	default:
		break;
	}

	dev_dbg(dev, "opcode: %#x sz_in: %zd sz_out: %zd rc: %d\n", cmd->opcode,
		cmd->size_in, cmd->size_out, rc);

	return rc;
}

static void label_area_release(void *lsa)
{
	vfree(lsa);
}

static void fw_buf_release(void *buf)
{
	vfree(buf);
}

#define CXL_MOCKMEM_RCD BIT(0)
#define CXL_MOCKMEM_TYPE2 BIT(1)

static bool is_rcd(struct platform_device *pdev)
{
	const struct platform_device_id *id = platform_get_device_id(pdev);

	return !!(id->driver_data & CXL_MOCKMEM_RCD);
}

static bool is_type2(struct platform_device *pdev)
{
	const struct platform_device_id *id = platform_get_device_id(pdev);

	return !!(id->driver_data & CXL_MOCKMEM_TYPE2);
}

static ssize_t event_trigger_store(struct device *dev,
				   struct device_attribute *attr,
				   const char *buf, size_t count)
{
	cxl_mock_event_trigger(dev);
	return count;
}
static DEVICE_ATTR_WO(event_trigger);

static void init_event_log(struct mock_event_log *log)
{
	rwlock_init(&log->lock);
	/* Handle can never be 0 use 1 based indexing for handle */
	log->cur_handle = 1;
	log->next_handle = 1;
}

static int __cxl_mock_mem_probe(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	struct cxl_memdev *cxlmd;
	struct cxl_memdev_state *mds;
	struct cxl_dev_state *cxlds;
	struct cxl_mockmem_data *mdata;
	int rc;

	mdata = devm_kzalloc(dev, sizeof(*mdata), GFP_KERNEL);
	if (!mdata)
		return -ENOMEM;
	dev_set_drvdata(dev, mdata);

	rc = cxl_mock_dc_region_setup(dev);
	if (rc)
		return rc;

	mdata->lsa = vmalloc(LSA_SIZE);
	if (!mdata->lsa)
		return -ENOMEM;
	mdata->fw = vmalloc(FW_SIZE);
	if (!mdata->fw)
		return -ENOMEM;
	mdata->fw_slot = 2;

	rc = devm_add_action_or_reset(dev, label_area_release, mdata->lsa);
	if (rc)
		return rc;

	rc = devm_add_action_or_reset(dev, fw_buf_release, mdata->fw);
	if (rc)
		return rc;

	mds = cxl_memdev_state_create(dev);
	if (IS_ERR(mds))
		return PTR_ERR(mds);

	mds->mbox_send = cxl_mock_mbox_send;
	mds->payload_size = SZ_4K;
	mds->event.buf = (struct cxl_get_event_payload *) mdata->event_buf;

	cxlds = &mds->cxlds;
	cxlds->serial = pdev->id;
	if (is_rcd(pdev)) {
		cxlds->rcd = true;
		cxlds->component_reg_phys = CXL_RESOURCE_NONE;
	}

	rc = cxl_enumerate_cmds(mds);
	if (rc)
		return rc;

	rc = cxl_poison_state_init(mds);
	if (rc)
		return rc;

	rc = cxl_set_timestamp(mds);
	if (rc)
		return rc;

	cxlds->media_ready = true;
	rc = cxl_dev_state_identify(mds);
	if (rc)
		return rc;

	rc = cxl_dev_dynamic_capacity_identify(mds);
	if (rc)
		return rc;

	rc = cxl_mem_create_range_info(mds);
	if (rc)
		return rc;

	mdata->mes.mds = mds;
	for (int i = 0; i < CXL_EVENT_TYPE_MAX; i++)
		init_event_log(&mdata->mes.mock_logs[i]);
	cxl_mock_add_event_logs(mdata);

	cxlmd = devm_cxl_add_memdev(cxlds);
	if (IS_ERR(cxlmd))
		return PTR_ERR(cxlmd);

	rc = cxl_dev_get_dynamic_capacity_extents(mds);
	if (rc)
		return rc;

	rc = cxl_memdev_setup_fw_upload(mds);
	if (rc)
		return rc;

	cxl_mem_get_event_records(mds, CXLDEV_EVENT_STATUS_ALL);

	return 0;
}

static int cxl_mock_type2_probe(struct platform_device *pdev)
{
	struct cxl_endpoint_decoder *cxled;
	struct device *dev = &pdev->dev;
	struct cxl_root_decoder *cxlrd;
	struct cxl_dev_state *cxlds;
	struct cxl_port *endpoint;
	struct cxl_memdev *cxlmd;
	resource_size_t max = 0;
	int rc;

	cxlds = cxl_accel_state_create(dev);
	if (IS_ERR(cxlds))
		return PTR_ERR(cxlds);

	cxlds->serial = pdev->id;
	cxlds->component_reg_phys = CXL_RESOURCE_NONE;
	cxlds->dpa_res = DEFINE_RES_MEM(0, DEV_SIZE);
	cxlds->ram_res = DEFINE_RES_MEM_NAMED(0, DEV_SIZE, "ram");
	cxlds->pmem_res = DEFINE_RES_MEM_NAMED(DEV_SIZE, 0, "pmem");
	if (is_rcd(pdev))
		cxlds->rcd = true;

	rc = request_resource(&cxlds->dpa_res, &cxlds->ram_res);
	if (rc)
		return rc;

	cxlmd = devm_cxl_add_memdev(cxlds);
	if (IS_ERR(cxlmd))
		return PTR_ERR(cxlmd);

	endpoint = cxl_acquire_endpoint(cxlmd);
	if (IS_ERR(endpoint))
		return PTR_ERR(endpoint);

	cxlrd = cxl_hpa_freespace(endpoint, &endpoint->host_bridge, 1,
				  CXL_DECODER_F_RAM | CXL_DECODER_F_TYPE2,
				  &max);

	if (IS_ERR(cxlrd)) {
		rc = PTR_ERR(cxlrd);
		goto out;
	}

	cxled = cxl_request_dpa(endpoint, CXL_DECODER_RAM, 0, max);
	if (IS_ERR(cxled)) {
		rc = PTR_ERR(cxled);
		goto out_cxlrd;
	}

	/* A real driver would do something with the returned region */
	rc = PTR_ERR_OR_ZERO(cxl_create_region(cxlrd, &cxled, 1));

	put_device(cxled_dev(cxled));
out_cxlrd:
	put_device(cxlrd_dev(cxlrd));
out:
	cxl_release_endpoint(cxlmd, endpoint);

	return rc;
}

static int cxl_mock_mem_probe(struct platform_device *pdev)
{
	if (is_type2(pdev))
		return cxl_mock_type2_probe(pdev);
	return __cxl_mock_mem_probe(pdev);
}

static ssize_t security_lock_show(struct device *dev,
				  struct device_attribute *attr, char *buf)
{
	struct cxl_mockmem_data *mdata = dev_get_drvdata(dev);

	return sysfs_emit(buf, "%u\n",
			  !!(mdata->security_state & CXL_PMEM_SEC_STATE_LOCKED));
}

static ssize_t security_lock_store(struct device *dev, struct device_attribute *attr,
				   const char *buf, size_t count)
{
	struct cxl_mockmem_data *mdata = dev_get_drvdata(dev);
	u32 mask = CXL_PMEM_SEC_STATE_FROZEN | CXL_PMEM_SEC_STATE_USER_PLIMIT |
		   CXL_PMEM_SEC_STATE_MASTER_PLIMIT;
	int val;

	if (kstrtoint(buf, 0, &val) < 0)
		return -EINVAL;

	if (val == 1) {
		if (!(mdata->security_state & CXL_PMEM_SEC_STATE_USER_PASS_SET))
			return -ENXIO;
		mdata->security_state |= CXL_PMEM_SEC_STATE_LOCKED;
		mdata->security_state &= ~mask;
	} else {
		return -EINVAL;
	}
	return count;
}

static DEVICE_ATTR_RW(security_lock);

static ssize_t fw_buf_checksum_show(struct device *dev,
				    struct device_attribute *attr, char *buf)
{
	struct cxl_mockmem_data *mdata = dev_get_drvdata(dev);
	u8 hash[SHA256_DIGEST_SIZE];
	unsigned char *hstr, *hptr;
	struct sha256_state sctx;
	ssize_t written = 0;
	int i;

	sha256_init(&sctx);
	sha256_update(&sctx, mdata->fw, mdata->fw_size);
	sha256_final(&sctx, hash);

	hstr = kzalloc((SHA256_DIGEST_SIZE * 2) + 1, GFP_KERNEL);
	if (!hstr)
		return -ENOMEM;

	hptr = hstr;
	for (i = 0; i < SHA256_DIGEST_SIZE; i++)
		hptr += sprintf(hptr, "%02x", hash[i]);

	written = sysfs_emit(buf, "%s\n", hstr);

	kfree(hstr);
	return written;
}

static DEVICE_ATTR_RO(fw_buf_checksum);

/* Returns if the proposed extent is valid */
static bool new_extent_valid(struct device *dev, size_t new_start,
			     size_t new_len)
{
	struct cxl_mockmem_data *mdata = dev_get_drvdata(dev);
	struct cxl_dc_extent_data *extent;
	size_t new_end, i;

	if (!new_len)
		return -EINVAL;

	new_end = new_start + new_len;

	dev_dbg(dev, "New extent %zx-%zx\n", new_start, new_end);

	/* Overlap with other extent? */
	xa_for_each(&mdata->dc_extents, i, extent) {
		size_t ext_end = extent->dpa_start + extent->length;

		if (extent->dpa_start <= new_start && new_start < ext_end) {
			dev_err(dev, "Extent overlap: Start %llu ?<= %zx ?<= %zx\n",
				extent->dpa_start, new_start, ext_end);
			return false;
		}
		if (extent->dpa_start <= new_end && new_end < ext_end) {
			dev_err(dev, "Extent overlap: End %llx ?<= %zx ?<= %zx\n",
				extent->dpa_start, new_end, ext_end);
			return false;
		}
	}

	/* Ensure it is in a region and is valid for that regions block size */
	for (int i = 0; i < NUM_MOCK_DC_REGIONS; i++) {
		struct cxl_dc_region_config *dc_region = &mdata->dc_regions[i];
		size_t reg_start, reg_end;

		reg_start = le64_to_cpu(dc_region->region_base);
		reg_end = le64_to_cpu(dc_region->region_length);
		reg_end += reg_start;

		dev_dbg(dev, "Region %d: %zx-%zx\n", i, reg_start, reg_end);

		if (reg_start >= new_start && new_end < reg_end) {
			u64 block_size = le64_to_cpu(dc_region->region_block_size);

			if (new_start % block_size || new_len % block_size) {
				dev_err(dev, "Extent not aligned to block size: start %zx; len %zx; block_size 0x%llx\n",
					new_start, new_len, block_size);
				return false;
			}

			dev_dbg(dev, "Extent in region %d\n", i);
			return true;
		}
	}

	return false;
}

/*
 * Format <start>:<length>:<tag>
 *
 * start and length must be a multiple of the configured region block size.
 * Tag can be any string up to 16 bytes.
 *
 * Extents must be exclusive of other extents
 */
static ssize_t dc_inject_extent_store(struct device *dev,
				      struct device_attribute *attr,
				      const char *buf, size_t count)
{
	char *start_str __free(kfree) = kstrdup(buf, GFP_KERNEL);
	unsigned long long start, length;
	char *len_str, *tag_str;
	size_t buf_len = count;
	int rc;

	if (!start_str)
		return -ENOMEM;

	len_str = strnchr(start_str, buf_len, ':');
	if (!len_str) {
		dev_err(dev, "Extent failed to find len_str: %s\n", start_str);
		return -EINVAL;
	}

	*len_str = '\0';
	len_str += 1;
	buf_len -= strlen(start_str);

	tag_str = strnchr(len_str, buf_len, ':');
	if (!tag_str) {
		dev_err(dev, "Extent failed to find tag_str: %s\n", len_str);
		return -EINVAL;
	}
	*tag_str = '\0';
	tag_str += 1;

	if (kstrtoull(start_str, 0, &start)) {
		dev_err(dev, "Extent failed to parse start: %s\n", start_str);
		return -EINVAL;
	}
	if (kstrtoull(len_str, 0, &length)) {
		dev_err(dev, "Extent failed to parse length: %s\n", len_str);
		return -EINVAL;
	}

	if (!new_extent_valid(dev, start, length))
		return -EINVAL;

	rc = devm_add_extent(dev, start, length, tag_str);
	if (rc)
		return rc;

	return count;
}
static DEVICE_ATTR_WO(dc_inject_extent);

static ssize_t dc_del_extent_store(struct device *dev,
				   struct device_attribute *attr,
				   const char *buf, size_t count)
{
	unsigned long long start;
	int rc;

	if (kstrtoull(buf, 0, &start)) {
		dev_err(dev, "Extent failed to parse start value\n");
		return -EINVAL;
	}

	rc = dc_delete_extent(dev, start);
	if (rc)
		return rc;

	return count;
}
static DEVICE_ATTR_WO(dc_del_extent);

static ssize_t dc_force_del_extent_store(struct device *dev,
					 struct device_attribute *attr,
					 const char *buf, size_t count)
{
	struct cxl_mockmem_data *mdata = dev_get_drvdata(dev);
	unsigned long long start;
	void *ext;
	int rc;

	if (kstrtoull(buf, 0, &start)) {
		dev_err(dev, "Extent failed to parse start value\n");
		return -EINVAL;
	}

	ext = xa_erase(&mdata->dc_accepted_exts, start);
	if (ext)
		dev_dbg(dev, "Forcing remove of accepted extent: %llx\n",
			start);

	dev_dbg(dev, "Forcing delete of extent at %llx\n", start);
	rc = dc_delete_extent(dev, start);
	if (rc)
		return rc;

	return count;
}
static DEVICE_ATTR_WO(dc_force_del_extent);

static struct attribute *cxl_mock_mem_attrs[] = {
	&dev_attr_security_lock.attr,
	&dev_attr_event_trigger.attr,
	&dev_attr_fw_buf_checksum.attr,
	&dev_attr_dc_inject_extent.attr,
	&dev_attr_dc_del_extent.attr,
	&dev_attr_dc_force_del_extent.attr,
	NULL
};
ATTRIBUTE_GROUPS(cxl_mock_mem);

static const struct platform_device_id cxl_mock_mem_ids[] = {
	{ .name = "cxl_mem", 0 },
	{ .name = "cxl_rcd", CXL_MOCKMEM_RCD | CXL_MOCKMEM_TYPE2 },
	{ },
};
MODULE_DEVICE_TABLE(platform, cxl_mock_mem_ids);

static struct platform_driver cxl_mock_mem_driver = {
	.probe = cxl_mock_mem_probe,
	.id_table = cxl_mock_mem_ids,
	.driver = {
		.name = KBUILD_MODNAME,
		.dev_groups = cxl_mock_mem_groups,
		.groups = cxl_mock_mem_core_groups,
	},
};

module_platform_driver(cxl_mock_mem_driver);
MODULE_LICENSE("GPL v2");
MODULE_IMPORT_NS(CXL);
