#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <linux/nvdimm_acpi.h>

#include "acpi_config.h"

#define MAX_CHAR_PER_LINE 256
#define g_malloc0 malloc

#define NVDIMM_FIT_CHR 'F'
#define NVDIMM_INTERLEAVE_CHR 'I'
#define NVDIMM_MEMDEV_CHR 'M'
#define NVDIMM_SPA_RNG_CHR 'S'
#define NVDIMM_FLUSH_TBL_CHR 'H'

#define NVDIMM_MAX_OFFSETS 16

#define NVDIMM_FITINFO_SIZE_OFFSET 4
#define NVDIMM_NUM_LINES_OFFSET 8
#define NVDIMM_INTERLEAVE_SIZE_OFFSET 4
#define NVDIMM_FLUSH_MAX_HINT_OFFSET 4
#define NVDIMM_FLUSH_VALID_HINT_OFFSET 6

static uint32_t nvdimm_fit_lens[] = {
4, /* header signature */
4, /* length in bytes */
1, /* revision */
1, /* checksum */
6, /* OEM ID */
8, /* OEM Table ID */
4, /* OEM revision */
4, /* creator ID */
4, /* creator revision */
2, /* number of spa range tables*/
2, /* number of mem dev to spa range tables*/
2, /* number of flush hint tables*/
2, /* number of interleave tables*/
2, /* size of flush hint tbl*/
2 /* reserved*/
};

static int nr_nvdimm_fit_fields = sizeof(nvdimm_fit_lens) / sizeof(uint32_t);

static uint32_t interleave_tbl_lens[] = {
2, /*interleave_idx*/
2, /*reserved*/
4, /*tbl_size*/
4, /*lines_described*/
4, /*line_size*/
4 /*Offset undefined number of offsets*/
};

static int nr_interleave_tbl_fields = sizeof(interleave_tbl_lens)
	/ sizeof(uint32_t);

static uint32_t memdev_spa_rng_lens[] = {
2, /*socket_id*/
2, /*mem ctrl id */
2, /*mem dev pid*/
2, /*mem dev lid*/
2, /*spa index*/
2, /*vendor id */
2, /* device id*/
2, /* revision id */
2, /* fmt interface code */
6, /* reserved */
8, /*length*/
8, /*region offset*/
8, /*spa start addr*/
2, /* interleave idx*/
1, /*interleave ways*/
};
static int nr_memdev_spa_rng_fields = sizeof(memdev_spa_rng_lens)
	/ sizeof(uint32_t);

static uint32_t spa_rng_lens[] = {
2, /*addr rng type*/
2, /* spa index*/
4, /* reserved */
8, /*start_addr*/
8 /*length*/
};

static int nr_spa_rng_fields = sizeof(spa_rng_lens) / sizeof(uint32_t);

static uint32_t flush_tbl_lens[] = {
2, /*socket id*/
2, /*mem ctrl id*/
2, /*max flush hint addrs*/
2, /*num valid flush addrs*/
8 /*flush hint addrs, undefined number*/
};

static int nr_flush_tbl_fields = sizeof(flush_tbl_lens) / sizeof(uint32_t);

int cur_offset;

static void __print_nvdimm_fit(struct nvdimm_fit *fit)
{
	fprintf(stderr, "***NVDIMM FIT***\n"
		"Signature: %u\n"
		"Length: %u\n"
		"Revision: %u\n"
		"Checksum: %u\n"
		"OEMID: %u%u%u%u%u%u\n"
		"OEM Table ID: %lu\n"
		"OEM Revision: %u\n"
		"Creator ID: %u\n"
		"Creator Revision: %u\n"
		"Number SPA RNG TBLs: %u\n"
		"Number of MEMDEV to SPA TBLs: %u\n"
		"Number of Flush Hint TBLS: %u\n"
		"Number of Interleave TBLs: %u\n"
		"Size of Flush Hint TBLs: %u\n"
		"Reserved: %u\n",
		fit->signature,
		fit->length,
		fit->revision,
		fit->checksum,
		fit->oemid[0], fit->oemid[1], fit->oemid[2],
		fit->oemid[3], fit->oemid[4], fit->oemid[5],
		(uint64_t) fit->oem_tbl_id,
		fit->oem_revision,
		fit->creator_id,
		fit->creator_revision,
		fit->num_spa_range_tbl,
		fit->num_memdev_spa_range_tbl,
		fit->num_flush_hint_tbl,
		fit->num_interleave_tbl,
		fit->size_flush_hint_tbl,
		fit->reserved);
}

void print_spa_rng_tbl(struct spa_rng_tbl *spa_rng, int spa_tbl_num)
{
	fprintf(stderr, "\t***SPA RANGE TBL %d Information***\n"
		"Address Range Type: %u\n"
		"SPA Index: %u\n"
		"Reserved: %u\n"
		"SPA Start Address: %lu\n"
		"SPA Window Length: %lu\n",
		spa_tbl_num,
		spa_rng->addr_rng_type,
		spa_rng->spa_index,
		spa_rng->reserved,
		(uint64_t) spa_rng->start_addr,
		(uint64_t) spa_rng->length);
}

void print_memdev_spa_rng_tbl(struct memdev_spa_rng_tbl *memdev_tbl,
	int memdev_tbl_num)
{
	fprintf(stderr, "\t***MEMDEV to SPA RNG TBL %d info***\n"
		"Socket ID: %hu\n"
		"MemCtrl ID: %hu\n"
		"Memory Device PID: %u\n"
		"Memory Device LID: %u\n"
		"SPA Index: %u\n"
		"Vendor ID: %u\n"
		"Device ID: %u\n"
		"Revision ID: %u\n"
		"Format Interface Code: %u\n"
		"Reserved: %u%u%u%u%u%u\n"
		"Length: %lu\n"
		"RegionOffset: %lu\n"
		"SPA Start Address: %lu\n"
		"Interleave Index: %u\n"
		"Interleave Ways: %u\n",
		memdev_tbl_num,
		memdev_tbl->socket_id,
		memdev_tbl->mem_ctrl_id,
		memdev_tbl->mem_dev_pid,
		memdev_tbl->mem_dev_lid,
		memdev_tbl->spa_index,
		memdev_tbl->vendor_id,
		memdev_tbl->device_id,
		memdev_tbl->rid,
		memdev_tbl->fmt_interface_code,
		memdev_tbl->reserved[0], memdev_tbl->reserved[1],
		memdev_tbl->reserved[2], memdev_tbl->reserved[3],
		memdev_tbl->reserved[4], memdev_tbl->reserved[5],
		(uint64_t) memdev_tbl->length,
		(uint64_t) memdev_tbl->region_offset,
		(uint64_t) memdev_tbl->start_spa,
		memdev_tbl->interleave_idx,
		memdev_tbl->interleave_ways);
}

void print_interleave_tbl(struct interleave_tbl *interleave,
	int interleave_tbl_num)
{
	int i;
	fprintf(stderr, "\t***Interleave TBL %d Information***\n"
		"InterleaveIndex: %u\n"
		"Reserved: %u\n"
		"TableSize: %u\n"
		"Lines Described: %u\n"
		"Line Size: %u\n",
		interleave_tbl_num,
		interleave->interleave_idx,
		interleave->reserved,
		interleave->tbl_size,
		interleave->lines_described,
		interleave->line_size);

	for (i = 0; i < interleave->lines_described; i++) {
		fprintf(stderr, "Offset%d: %u\n", i,
			(uint32_t) *((uint32_t *) &(interleave->offsets) + i));
	}
}

void print_flush_hint_tbl(struct flush_hint_tbl *flush_tbl, int tbl_num)
{
	int i;
	fprintf(stderr, "\t***Flush Hint TBL %d Information***\n"
		"Socket ID: %hu\n"
		"MemCtrl ID: %hu\n"
		"Max Flush Hint Addrs: %hu\n"
		"Valid Flush Addrs: %hu\n",
		tbl_num,
		flush_tbl->socket_id,
		flush_tbl->mem_ctrl_id,
		flush_tbl->max_flush_hint_addrs,
		flush_tbl->num_valid_flush_hint_addrs);

	for (i = 0; i < flush_tbl->max_flush_hint_addrs; i++) {
		fprintf(stderr, "Flush Hint Addr#%d: %#lx\n", i,
			(uint64_t) *((uint64_t *) &(flush_tbl->flush_hint_addr)
				+ i));
	}
}

void print_nvdimm_fit(void *device_info)
{
	uint16_t num_spa_rng_tbls = 0;
	uint16_t num_memdev_spa_tbls = 0;
	uint16_t num_interleave_tbls = 0;
	uint16_t num_flush_tbls = 0;
	uint16_t flush_tbl_size = 0;
	int i;
	if (device_info == NULL)
		return;
	num_spa_rng_tbls =
		((struct nvdimm_fit *) device_info)->num_spa_range_tbl;
	num_memdev_spa_tbls =
		((struct nvdimm_fit *) device_info)->num_memdev_spa_range_tbl;
	num_flush_tbls =
		((struct nvdimm_fit *) device_info)->num_flush_hint_tbl;
	num_interleave_tbls =
		((struct nvdimm_fit *) device_info)->num_interleave_tbl;
	flush_tbl_size =
		((struct nvdimm_fit *) device_info)->size_flush_hint_tbl;
	__print_nvdimm_fit((struct nvdimm_fit *) device_info);
	device_info += sizeof(struct nvdimm_fit);

	for (i = 0; i < num_spa_rng_tbls; i++) {
		print_spa_rng_tbl((struct spa_rng_tbl *) device_info, i);
		device_info += sizeof(struct spa_rng_tbl);
	}

	for (i = 0; i < num_memdev_spa_tbls; i++) {
		print_memdev_spa_rng_tbl(
			(struct memdev_spa_rng_tbl *) device_info, i);
		device_info += sizeof(struct memdev_spa_rng_tbl);
	}

	for (i = 0; i < num_flush_tbls; i++) {
		print_flush_hint_tbl((struct flush_hint_tbl *) device_info, i);
		device_info += flush_tbl_size;
	}

	for (i = 0; i < num_interleave_tbls; i++) {
		struct interleave_tbl *tmp =
			(struct interleave_tbl *) device_info;
		print_interleave_tbl(tmp, i);
		device_info += sizeof(*tmp) - (sizeof(tmp->offsets))
			+ (tmp->lines_described * sizeof(uint32_t));
	}
}

static void put_data(void *dev_info, int *offset, int size, char *val)
{
	if (size == 1)
		*(uint8_t *) (dev_info + *offset) = (uint8_t) strtol(val, NULL,
			10);
	else if (size == 2)
		*(uint16_t *) (dev_info + *offset) = (uint16_t) strtol(val,
			NULL, 10);
	else if (size == 4)
		*(uint32_t *) (dev_info + *offset) = (uint32_t) strtol(val,
			NULL, 10);
	else if (size == 6) {
		/* XXX: I honestly don't know how to interpret this */
		/* I think that it is just a byte code - so copy the bytes */
		int i;
		for (i = 0; i <= 5; i++)
			*(uint8_t *) (dev_info + *offset + i) = *(val + i);
	} else if (size == 8)
		*(uint64_t *) (dev_info + *offset) = strtoll(val, NULL, 10);
	else
		LOG_DBG("Illegal field length specified - ignored");

	*offset += size;
}

static void put_f(void *dev_info, int *offset, int size)
{
	if (size == 1)
		*(uint8_t *) (dev_info + *offset) = 0xff;
	else if (size == 2)
		*(uint16_t *) (dev_info + *offset) = 0xffff;
	else if (size == 4)
		*(uint32_t *) (dev_info + *offset) = 0xffffffff;
	else if (size == 6) {
		/* XXX: I honestly don't know how to interpret this */
		/* I think that it is just a byte code - so copy the bytes */
		int i;
		for (i = 0; i <= 5; i++)
			*(uint8_t *) (dev_info + *offset + i) = 0xff;
	} else if (size == 8)
		*(uint64_t *) (dev_info + *offset) = 0xffffffffffffffff;
	else
		LOG_DBG("Illegal field length specified - ignored");

	*offset += size;
}

static int parse_line(int nr_fields, uint32_t field_lens[], int *cur_offset,
	void *dev_info)
{
	char *pch;
	int field_nr = 0;

	pch = strtok(NULL, " \t");
	while (pch != NULL && field_nr < nr_fields) {
		put_data(dev_info, cur_offset, field_lens[field_nr++], pch);
		pch = strtok(NULL, " \t");
	}

	if (pch != NULL || field_nr != nr_fields)
		return 1;

	return 0;
}

static int parse_interleave_line(int nr_fields, uint32_t field_lens[],
	int *cur_offset, void *dev_info)
{
	char *pch;
	int field_nr = 0;
	int tbl_offset = *cur_offset;
	int num_lines;
	int i;
	pch = strtok(NULL, " \t");
	while (pch != NULL && field_nr < nr_fields - 1) {
		put_data(dev_info, cur_offset, field_lens[field_nr++], pch);
		pch = strtok(NULL, " \t");
	}

	num_lines = *(uint32_t *) (dev_info + tbl_offset
		+ NVDIMM_NUM_LINES_OFFSET);

	for (i = 0; i < num_lines; i++) {
		put_data(dev_info, cur_offset, field_lens[field_nr], pch);
		pch = strtok(NULL, " \t");
	}

	if (pch != NULL || field_nr != (nr_fields - 1))
		return 1;

	return 0;
}

static int parse_flush_hint_line(int nr_fields, uint32_t field_lens[],
	int *cur_offset, void *dev_info)
{
	char *pch;
	int field_nr = 0;
	int tbl_offset = *cur_offset;
	uint16_t max_lines;
	uint16_t valid_lines;
	int i;
	pch = strtok(NULL, " \t");
	while (pch != NULL && field_nr < nr_fields - 1) {
		put_data(dev_info, cur_offset, field_lens[field_nr++], pch);
		pch = strtok(NULL, " \t");
	}

	max_lines = *(uint16_t *) (dev_info + tbl_offset
		+ NVDIMM_FLUSH_MAX_HINT_OFFSET);
	valid_lines = *(uint16_t *) (dev_info + tbl_offset
		+ NVDIMM_FLUSH_VALID_HINT_OFFSET);

	for (i = 0; i < valid_lines; i++) {
		put_data(dev_info, cur_offset, field_lens[field_nr], pch);
		pch = strtok(NULL, " \t");
	}
	for (i = max_lines - valid_lines; i > 0; i--)
		put_f(dev_info, cur_offset, field_lens[field_nr]);


	if (pch != NULL || field_nr != (nr_fields - 1))
		return 1;

	return 0;
}

static int parse_fit_tbl(int *cur_offset, void *dev_info)
{
	return parse_line(nr_nvdimm_fit_fields, nvdimm_fit_lens, cur_offset,
		dev_info);
}

static int parse_spa_rng_tbl(int *cur_offset, void *dev_info)
{
	return parse_line(nr_spa_rng_fields, spa_rng_lens, cur_offset,
		dev_info);
}

static int parse_memdev_spa_rng_tbl(int *cur_offset, void *dev_info)
{
	return parse_line(nr_memdev_spa_rng_fields, memdev_spa_rng_lens,
		cur_offset, dev_info);
}

static int parse_interleave_tbl(int *cur_offset, void *dev_info)
{
	return parse_interleave_line(nr_interleave_tbl_fields,
		interleave_tbl_lens, cur_offset, dev_info);
}

static int parse_flush_hint_tbl(int *cur_offset, void *dev_info)
{
	return parse_flush_hint_line(nr_flush_tbl_fields, flush_tbl_lens,
		cur_offset, dev_info);
}

static int parse_nvdimm_fit_line(char *line, int *cur_offset, void *dev_info)
{
	char *pch;
	uint32_t tbl_start;

	pch = strtok(line, " \t");
	if (pch == NULL
		|| (pch[0] != NVDIMM_FIT_CHR && pch[0] != NVDIMM_SPA_RNG_CHR
			&& pch[0] != NVDIMM_MEMDEV_CHR
			&& pch[0] != NVDIMM_INTERLEAVE_CHR
			&& pch[0] != NVDIMM_FLUSH_TBL_CHR) != 0)
		return 1;
	switch (pch[0]) {
	case NVDIMM_FIT_CHR:
		if (parse_fit_tbl(cur_offset, dev_info) != 0) {
			LOG_ERR("Config FIT line invalid");
			return 1;
		}
		break;
	case NVDIMM_SPA_RNG_CHR:
		if (parse_spa_rng_tbl(cur_offset, dev_info) != 0) {
			LOG_ERR("Config SPA RANGE info invalid");
			return 1;
		}
		break;
	case NVDIMM_MEMDEV_CHR:
		if (parse_memdev_spa_rng_tbl(cur_offset, dev_info) != 0) {
			LOG_ERR("Config MEMDEV number invalid");
			return 1;
		}
		break;
	case NVDIMM_INTERLEAVE_CHR:
		tbl_start = *cur_offset;
		if (parse_interleave_tbl(cur_offset, dev_info) != 0) {
			LOG_ERR("Config Interleave info invalid");
			return 1;
		}

		*(uint32_t *) (dev_info + tbl_start
			+ NVDIMM_INTERLEAVE_SIZE_OFFSET) = *cur_offset
			- tbl_start;
		break;
	case NVDIMM_FLUSH_TBL_CHR:
		if (parse_flush_hint_tbl(cur_offset, dev_info) != 0) {
			LOG_ERR("Config Flush Hint info invalid");
			return 1;
		}
		break;
	default:
		LOG_ERR("Config File Invalid Leading Line Character");
		return 1;
	}

	return 0;
}

void *create_test_fit(char *fit_str[])
{
	int cur_offset = 0;
	int i;
	char line[MAX_CHAR_PER_LINE];

	void *device_info = (void *) g_malloc0(NVDIMM_CONFIG_SIZE);

	int fit_str_size = ARRAY_SIZE(fit_str);

	for (i = 0; i < fit_str_size; i++) {
		int err = 0;
		strcpy(line, fit_str[i]);
		err = parse_nvdimm_fit_line(line, &cur_offset, device_info);
		if (err)
			return NULL ;
	}

	*(uint32_t *) (device_info + NVDIMM_FITINFO_SIZE_OFFSET) =
		(uint32_t) cur_offset;

	return device_info;
}

/*
 * Create a default NVDIMM Firmware Interface Table
 * It is setup as follows:
 * 3 Sockets
 * Socket 1:
 *	6 DIMMs(2 iMC)
 *		1GB of PM per DIMM
 *			4k Interleave
 *		7M of Mailbox
 *			256 Interleave
 *		122M of BlockWindow
 *			256 Interleave
 *
 * Socket 2:
 *	6 DIMMs(2 iMC)
 *		1GB of PM per DIMM
 *			4k Interleave
 *		7M of Mailbox per DIMM
 *			256 Interleave
 *		122M of BlockWindow per DIMM
 *			256 Interleave
 *
 * Socket 3:
 *	3 DIMMs (One iMC)
 *		1GB of PM per DIMM
 *			4k Interleave
 *		7M of Mailbox per DIMM
 *			256 Interleave
 *		122M of BlockWindow per DIMM
 *			256 Interleave
 */
void *create_default_fit(void *dev_info)
{
	int cur_offset = 0;
	int i;
	char line[MAX_CHAR_PER_LINE];
	static char *default_nvdimm_fit[] = {
	"F 609043789 0 2 4 abcdef 5 6 7 8 9 45 5 15 104 0",
	/*Socket 1 SPA RNG Tables*/
	"S 1 2 0 817889280 6442450944",
	"S 2 0 0 6291456 44040192",
	"S 3 1 0 50331648 767557632",
	/*Socket 2 SPA RNG Tables*/
	"S 1 5 0 8071938048 6442450944",
	"S 2 3 0 7260340224 44040192",
	"S 3 4 0 7304380416 767557632",
	/*Socket 3 SPA RNG Tables*/
	"S 1 8 0 14920187904 3221225472",
	"S 2 6 0 14514388992 22020096",
	"S 3 7 0 14536409088 383778816",
	/*Socket 1 Memdev to SPA Tables*/
	"M 1 0 1 0 0 32896 1234 0 1 000000 7340032 0 6291456 1 6",
	"M 1 0 2 0 0 32896 1234 0 1 000000 7340032 0 6291456 2 6",
	"M 1 0 3 0 0 32896 1234 0 1 000000 7340032 0 6291456 3 6",
	"M 1 1 4 0 0 32896 1234 0 1 000000 7340032 0 6291456 4 6",
	"M 1 1 5 0 0 32896 1234 0 1 000000 7340032 0 6291456 5 6",
	"M 1 1 6 0 0 32896 1234 0 1 000000 7340032 0 6291456 6 6",
	"M 1 0 1 1 1 32896 1234 0 1 000000 127926272 0 50331648 1 6",
	"M 1 0 2 1 1 32896 1234 0 1 000000 127926272 0 50331648 2 6",
	"M 1 0 3 1 1 32896 1234 0 1 000000 127926272 0 50331648 3 6",
	"M 1 1 4 1 1 32896 1234 0 1 000000 127926272 0 50331648 4 6",
	"M 1 1 5 1 1 32896 1234 0 1 000000 127926272 0 50331648 5 6",
	"M 1 1 6 1 1 32896 1234 0 1 000000 127926272 0 50331648 6 6",
	"M 1 0 1 2 2 32896 1234 0 1 000000 1073741824 0 817889280 7 6",
	"M 1 0 2 2 2 32896 1234 0 1 000000 1073741824 0 817889280 8 6",
	"M 1 0 3 2 2 32896 1234 0 1 000000 1073741824 0 817889280 9 6",
	"M 1 1 4 2 2 32896 1234 0 1 000000 1073741824 0 817889280 10 6",
	"M 1 1 5 2 2 32896 1234 0 1 000000 1073741824 0 817889280 11 6",
	"M 1 1 6 2 2 32896 1234 0 1 000000 1073741824 0 817889280 12 6",
	/*Socket 2 Memdev to SPA Tables*/
	"M 2 0 7 0 3 32896 1234 0 1 000000 7340032 0 7260340224 1 6",
	"M 2 0 8 0 3 32896 1234 0 1 000000 7340032 0 7260340224 2 6",
	"M 2 0 9 0 3 32896 1234 0 1 000000 7340032 0 7260340224 3 6",
	"M 2 1 10 0 3 32896 1234 0 1 000000 7340032 0 7260340224 4 6",
	"M 2 1 11 0 3 32896 1234 0 1 000000 7340032 0 7260340224 5 6",
	"M 2 1 12 0 3 32896 1234 0 1 000000 7340032 0 7260340224 6 6",
	"M 2 0 7 1 4 32896 1234 0 1 000000 127926272 0 7304380416 1 6",
	"M 2 0 8 1 4 32896 1234 0 1 000000 127926272 0 7304380416 2 6",
	"M 2 0 9 1 4 32896 1234 0 1 000000 127926272 0 7304380416 3 6",
	"M 2 1 10 1 4 32896 1234 0 1 000000 127926272 0 7304380416 4 6",
	"M 2 1 11 1 4 32896 1234 0 1 000000 127926272 0 7304380416 5 6",
	"M 2 1 12 1 4 32896 1234 0 1 000000 127926272 0 7304380416 6 6",
	"M 2 0 7 2 5 32896 1234 0 1 000000 1073741824 0 8071938048 7 6",
	"M 2 0 8 2 5 32896 1234 0 1 000000 1073741824 0 8071938048 8 6",
	"M 2 0 9 2 5 32896 1234 0 1 000000 1073741824 0 8071938048 9 6",
	"M 2 1 10 2 5 32896 1234 0 1 000000 1073741824 0 8071938048 10 6",
	"M 2 1 11 2 5 32896 1234 0 1 000000 1073741824 0 8071938048 11 6",
	"M 2 1 12 2 5 32896 1234 0 1 000000 1073741824 0 8071938048 12 6",
	/*Socket 3 Memdev to SPA Tables*/
	"M 3 0 13 0 6 32896 1234 0 1 000000 7340032 0 14514388992 13 3",
	"M 3 0 14 0 6 32896 1234 0 1 000000 7340032 0 14514388992 14 3",
	"M 3 0 15 0 6 32896 1234 0 1 000000 7340032 0 14514388992 15 3",
	"M 3 0 13 1 7 32896 1234 0 1 000000 127926272 0 14536409088 13 3",
	"M 3 0 14 1 7 32896 1234 0 1 000000 127926272 0 14536409088 14 3",
	"M 3 0 15 1 7 32896 1234 0 1 000000 127926272 0 14536409088 15 3",
	"M 3 0 13 2 8 32896 1234 0 1 000000 1073741824 0 14920187904 7 3",
	"M 3 0 14 2 8 32896 1234 0 1 000000 1073741824 0 14920187904 8 3",
	"M 3 0 15 2 8 32896 1234 0 1 000000 1073741824 0 14920187904 9 3",
	/*Socket 1 Flush Tables*/
	"H 1 0 12 3 1 2 3",
	"H 1 1 12 3 4 5 6",
	/*Socket 2 Flush Tables*/
	"H 2 0 12 3 7 8 9",
	"H 2 1 12 3 10 11 12",
	/*Socket 3 Flush Tables*/
	"H 3 0 12 3 13 14 15",

	"I 1 0 0 16 256 0 768 1536 2304 3072 3840 8448 9216 9984 10752"
	" 11520 16896 17664 18432 19200 19968",
	"I 2 0 0 16 256 256 1024 1792 2560 3328 8704 9472 10240 11008"
	" 11776 16384 17152 17920 18688 19456 20224",
	"I 3 0 0 16 256 512 1280 2048 2816 3584 8192 8960 9728 10496 11264"
	" 12032 16640 17408 18176 18944 19712",
	"I 4 0 0 16 256 4608 5376 6144 6912 7680 12288 13056 13824 14592"
	" 15360 16128 20736 21504 22272 23040 23808",
	"I 5 0 0 16 256 4096 4864 5632 6400 7168 7936 12544 13312 14080"
	" 14848 15616 20992 21760 22528 23296 24064",
	"I 6 0 0 16 256 4352 5120 5888 6656 7424 12800 13568 14336 15104"
	" 15872 20480 21248 22016 22784 23552 24320",
	"I 7 0 0 1 4096 X",
	"I 8 0 0 1 4096 X",
	"I 9 0 0 1 4096 X",
	"I 10 0 0 1 4096 X",
	"I 11 0 0 1 4096 X",
	"I 12 0 0 1 4096 X",
	"I 13 0 0 3 256 0 768 1536",
	"I 14 0 0 3 256 256 1024 1792",
	"I 15 0 0 3 256 512 1280 2048" };

	static int default_size = sizeof(default_nvdimm_fit) / sizeof(char *);

	for (i = 0; i < default_size; i++) {
		int err = 0;
		strcpy(line, default_nvdimm_fit[i]);
		err = parse_nvdimm_fit_line(line, &cur_offset, dev_info);
		if (err)
			return NULL ;
	}

	*(uint32_t *) (dev_info + NVDIMM_FITINFO_SIZE_OFFSET) =
		(uint32_t) cur_offset;

	return dev_info;
}

void *read_nvdimm_fit_file(char *config_nm)
{
	/* Temp array used to store a line in the file */
	char var_line[MAX_CHAR_PER_LINE];
	void *device_info;
	int cur_offset = 0;
	FILE *config_file;

	device_info = (void *) g_malloc0(NVDIMM_CONFIG_SIZE);

	if (config_nm == NULL || config_nm[0] == '\0') {
		LOG_DBG("FIT Config File not specified");
		return create_default_fit(device_info);
	}

	config_file = fopen(config_nm, "r");
	/* Create the default nvdimm fit if configuration file doesn't exist */
	if (config_file == NULL) {
		LOG_DBG("FIT Config File %s not found", config_nm);
		return create_default_fit(device_info);
	}

	while (fgets(var_line, MAX_CHAR_PER_LINE, config_file) != NULL) {
		int err = 0;
		err = parse_nvdimm_fit_line(var_line, &cur_offset,
			device_info);
		if (err) {
			fclose(config_file);
			return create_default_fit(device_info);
		}
	}

	if (!feof(config_file)) {
		LOG_DBG("I/O error reading FIT config file");
		fclose(config_file);
		return create_default_fit(device_info);
	}

	/* fill in the length of the NVDIMM FIT structure */
	*(uint32_t *) (device_info + NVDIMM_FITINFO_SIZE_OFFSET) =
		(uint32_t) cur_offset;

	fclose(config_file);

	return device_info;
}


