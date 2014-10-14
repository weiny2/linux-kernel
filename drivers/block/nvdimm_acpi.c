/*
 * Copyright 2013 Intel Corporation All Rights Reserved.
 *
 * INTEL CONFIDENTIAL
 *
 * The source code contained or described herein and all documents related to
 * the source code ("Material") are owned by Intel Corporation or its suppliers
 * or licensors. Title to the Material remains with Intel Corporation or its
 * suppliers and licensors. The Material may contain trade secrets and
 * proprietary and confidential information of Intel Corporation and its
 * suppliers and licensors, and is protected by worldwide copyright and trade
 * secret laws and treaty provisions. No part of the Material may be used,
 * copied, reproduced, modified, published, uploaded, posted, transmitted,
 * distributed, or disclosed in any way without Intel's prior express written
 * permission.
 *
 * No license under any patent, copyright, trade secret or other intellectual
 * property right is granted to or conferred upon you by disclosure or delivery
 * of the Materials, either expressly, by implication, inducement, estoppel or
 * otherwise. Any license under such intellectual property rights must be
 * express and approved by Intel in writing.
 *
 * Unless otherwise agreed by Intel in writing, you may not remove or alter
 * this notice or any other notice embedded in Materials by Intel or Intel's
 * suppliers or licensors in any way.
 */

#include <linux/sort.h>
#include <linux/nvdimm_core.h>
#include <linux/nvdimm_acpi.h>

/* TODO: Change all sizeof(struct type) to sizeof(*variable)*/
/* TODO: Change all casting such as (__u64 *) to typeof(variable_name)*/

/**
 * spa_to_rdpa() - Convert System Physical Address to Device Region Physical
 * Address
 *
 * @spa: System Physical Address to convert
 * @mdsparng_tbl: The Memdev to SPA range that helps describe this region of
 * memory
 * @i_tbl: Interleave table referenced by the mdsparng_tbl
 * @err: Errors go here
 *
 * A memory device could have multiple regions. As such we cannot convert
 * to a device physical address. Instead we refer to the address for a region
 * within the device as device region physical address (RDPA), where rdpa is
 * a zero based address from the start of the region within the device.
 *
 * Generic
 *
 * Returns: RDPA address in a given region,
 * 0 and err = _EINVAL  if a divide by zero error &&
 * if the line number is not found
 */
__u64 spa_to_rdpa(phys_addr_t spa, struct memdev_spa_rng_tbl *mdsparng_tbl,
	struct interleave_tbl *i_tbl, int *err)
{
	long int line_number = -1;
	__u64 rotation_size;
	__u64 spa_prime;
	__u64 rotation_number;

	int i;
	if (i_tbl) {
		if (!i_tbl->line_size || !i_tbl->lines_described
			|| !mdsparng_tbl->interleave_ways
			|| (mdsparng_tbl->start_spa > spa)) {
			NVDIMM_DBG("Divide by Zero\n");
			*err = -ERANGE;
			return 0;
		}

		rotation_size = i_tbl->line_size * i_tbl->lines_described;
		spa_prime = ((spa - mdsparng_tbl->start_spa)
			% (rotation_size * mdsparng_tbl->interleave_ways))
			- (spa % i_tbl->line_size);

		rotation_number = (spa - mdsparng_tbl->start_spa)
			/ (rotation_size * mdsparng_tbl->interleave_ways);

		for (i = 0; i < i_tbl->lines_described; i++) {
			if (i_tbl->offsets[i] == spa_prime)
				line_number = i;
		}

		if (line_number < 0) {
			*err = -ERANGE;
			return 0;
		}

		return line_number * i_tbl->line_size
			+ (rotation_number * rotation_size)
			+ (spa % i_tbl->line_size);
	} else {
		if (mdsparng_tbl->start_spa > spa) {
			NVDIMM_DBG("Invalid SPA for conversion\n");
			*err = -ERANGE;
			return 0;
		} else
			return spa - mdsparng_tbl->start_spa;
	}
}
EXPORT_SYMBOL(spa_to_rdpa);

/**
 * rdpa_to_spa() - Convert Device Region Physical to System Physical
 * Address
 *
 * @rdpa: Device Region Physical Address to convert
 * @mdsparng_tbl: The Memdev to SPA range that helps describe this region of
 * memory
 * @i_tbl: Interleave table referenced by the mdsparng_tbl
 *
 * A memory device could have multiple regions. As such we cannot convert
 * to a device physical address. Instead we refer to the address for a region
 * within the device as device region physical address (RDPA), where rdpa is
 * a zero based address from the start of the region within the device.
 *
 * Generic
 *
 * Returns: SPA address, or 0 with err set to -EINVAL on a divide by
 * zero error
 */
phys_addr_t rdpa_to_spa(__u64 rdpa, struct memdev_spa_rng_tbl *mdsparng_tbl,
	struct interleave_tbl *i_tbl, int *err)
{
	__u64 rotation_size;
	__u64 rotation_num;
	__u32 line_num;
	if (i_tbl) {
		if (!i_tbl->line_size || !i_tbl->lines_described) {
			NVDIMM_DBG("Divide by Zero\n");
			*err = -ERANGE;
			return 0;
		}

		rotation_size = i_tbl->line_size * i_tbl->lines_described;
		rotation_num = rdpa / rotation_size;
		line_num = (rdpa % rotation_size) / i_tbl->line_size;

		return mdsparng_tbl->start_spa
			+ rotation_num * rotation_size
				* mdsparng_tbl->interleave_ways
			+ i_tbl->offsets[line_num] + rdpa % i_tbl->line_size;
	} else {
		return mdsparng_tbl->start_spa + rdpa;
	}
}
EXPORT_SYMBOL(rdpa_to_spa);

/**
 * __get_memdev_spa_tbl() - Get Memory Device to SPA Range Table by DIMM ID
 * and Range Type
 * @fit_head: NVDIMM FIT for the system
 * @pid: SMBIOS handle of the DIMM
 *
 * Finds the first memdev table that matches the physical ID
 *
 * Returns:
 * memdev_spa_rng_tbl on Success
 * NULL on Failure
 */
struct memdev_spa_rng_tbl *__get_memdev_spa_tbl(struct fit_header *fit_head,
	__u16 pid)
{
	struct memdev_spa_rng_tbl *memdev_tbls = fit_head->memdev_spa_rng_tbls;
	int i;

	if (!fit_head->memdev_spa_rng_tbls) {
		NVDIMM_ERR("No Memdev tables to search");
		/* TODO: Is bug the correct action? */
		BUG();
	}

	for (i = 0; i < fit_head->fit->num_memdev_spa_range_tbl; i++) {
		if (memdev_tbls[i].mem_dev_pid == pid)
			return &memdev_tbls[i];
	}

	return NULL;
}

/**
 * get_memdev_spa_tbl() - Get Memory Device to SPA Range Table by DIMM ID
 * and Range Type
 * @fit_head: NVDIMM FIT for the system
 * @pid: SMBIOS handle of the DIMM
 * @addr_rng_type: Range type to search for
 *
 * Search all the memdev to spa range tables and find the dimm that matches
 * the physical ID and range type
 *
 * Warning: This function can only be used if there is only 1 range of the
 * type you are looking for
 * Meaning if you are trying to match a PM Range type you better be sure there
 * can only ever be 1 PM Range type per NVDIMM.
 */
struct memdev_spa_rng_tbl *get_memdev_spa_tbl(struct fit_header *fit_head,
	__u16 pid, __u16 addr_rng_type)
{
	struct spa_rng_tbl *s_tbls = fit_head->spa_rng_tbls;
	struct memdev_spa_rng_tbl *memdev_tbls = fit_head->memdev_spa_rng_tbls;
	int i;

	if (!fit_head->memdev_spa_rng_tbls) {
		NVDIMM_DBG("No Memdev tables to search");
		/* TODO: Bug? */
		BUG();
	}

	for (i = 0; i < fit_head->fit->num_memdev_spa_range_tbl; i++) {
		if (memdev_tbls[i].mem_dev_pid == pid
			&& s_tbls[memdev_tbls[i].spa_index].addr_rng_type
				== addr_rng_type)
			return &memdev_tbls[i];
	}

	return NULL;
}
EXPORT_SYMBOL(get_memdev_spa_tbl);

/**
 * get_num_pm_dimms() - Calculate Number of PM DIMMs in FIT
 * @fit_head: Fully populated fit header
 *
 * Analyze the NVDIMM FIT and calculate the number of NVDIMMs that
 * contain a PM region
 *
 * Generic
 *
 * Returns: Number of PM DIMMs in system
 */
__u8 get_num_pm_dimms(struct fit_header *fit_head)
{

	__u8 num_pm_dimms = 0;
	struct memdev_spa_rng_tbl *memdev_tbls = fit_head->memdev_spa_rng_tbls;
	struct spa_rng_tbl *spa_rng_tbls = fit_head->spa_rng_tbls;
	int i, j;
	int empty_index = 0;
	__u16 *unique_pids = kcalloc(fit_head->fit->num_memdev_spa_range_tbl,
			sizeof(__u16), GFP_KERNEL);
	if (!unique_pids)
		return -ENOMEM;

	/*
	 * Scan each memdev tbl, if memdev tbl points to a PM spa region,
	 * check and see if we have already recorded this dimm as a PM DIMM
	 */
	for (i = 0; i < fit_head->fit->num_memdev_spa_range_tbl; i++) {
		if (spa_rng_tbls[memdev_tbls[i].spa_index].addr_rng_type
			== NVDIMM_PM_RNG_TYPE) {
			int duplicate_found = 0;
			for (j = 0; j < empty_index; j++) {
				if (memdev_tbls[i].mem_dev_pid
					== unique_pids[j]) {
					duplicate_found = 1;
				}
			}
			if (!duplicate_found) {
				num_pm_dimms++;
				unique_pids[empty_index] =
					memdev_tbls[i].mem_dev_pid;
				empty_index++;
			}
		}
	}

	kfree(unique_pids);
	return num_pm_dimms;
}

/**
 * get_num_dimms() - Calculate number of DIMMs
 * @memdev_tbls: The memory device tables that describe the DIMMs in the FIT
 * @num_tbls: Number of memory device tables in the FIT
 *
 * Analyze the fit header and determine the number of NVM DIMMs present in the
 * FIT
 *
 * Generic
 *
 * Returns: Number of NVM DIMMs in the FIT
 */
__u8 get_num_dimms(struct memdev_spa_rng_tbl *memdev_tbls, __u16 num_tbls)
{
	__u8 num_unique_dimms = 0;
	int empty_index = 0;
	int i, j;
	__u16 *unique_pids = kcalloc(num_tbls, sizeof(__u16), GFP_KERNEL);

	if (!unique_pids)
		return -ENOMEM;

	for (i = 0; i < num_tbls; i++) {
		int duplicate_found = 0;
		for (j = 0; j < empty_index; j++) {
			if (memdev_tbls[i].mem_dev_pid == unique_pids[j])
				duplicate_found = 1;

		}
		if (!duplicate_found) {
			num_unique_dimms++;
			unique_pids[empty_index] = memdev_tbls[i].mem_dev_pid;
			empty_index++;
		}
	}

	kfree(unique_pids);
	return num_unique_dimms;
}

static void print_nvdimm_fit(struct nvdimm_fit *fit)
{
	NVDIMM_VDBG("Signature: %u\n"
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
		"Reserved: %u",
		fit->signature,
		fit->length,
		fit->revision,
		fit->checksum,
		fit->oemid[0], fit->oemid[1], fit->oemid[2],
		fit->oemid[3], fit->oemid[4], fit->oemid[5],
		(unsigned long)fit->oem_tbl_id,
		fit->oem_revision,
		fit->creator_id,
		fit->creator_revision,
		fit->num_spa_range_tbl,
		fit->num_memdev_spa_range_tbl,
		fit->num_flush_hint_tbl,
		fit->num_interleave_tbl,
		fit->size_flush_hint_tbl,
		fit->reserved
		);
}

static void print_spa_rng_tbl(struct spa_rng_tbl *spa_rng, int spa_tbl_num)
{
	NVDIMM_VDBG("\n***SPA RANGE TBL %d Information***\n"
		"Address Range Type: %u\n"
		"SPA Index: %u\n"
		"Reserved: %u\n"
		"SPA Start Address: %lu\n"
		"SPA Window Length: %lu",
		spa_tbl_num,
		spa_rng->addr_rng_type,
		spa_rng->spa_index,
		spa_rng->reserved,
		(unsigned long)spa_rng->start_addr,
		(unsigned long)spa_rng->length
	);
}

static void print_memdev_spa_rng_tbl(
	struct memdev_spa_rng_tbl *memdev_tbl, int memdev_tbl_num)
{
	NVDIMM_VDBG("\n***MEMDEV to SPA RNG TBL %d info***\n"
		"Socket ID: %u\n"
		"MemCtrl ID: %u\n"
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
		"Interleave Ways: %u",
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
		(unsigned long)memdev_tbl->length,
		(unsigned long)memdev_tbl->region_offset,
		(unsigned long)memdev_tbl->start_spa,
		memdev_tbl->interleave_idx,
		memdev_tbl->interleave_ways
	);
}

static void print_flush_hint_tbl(struct flush_hint_tbl *flush_tbl,
	int tbl_num)
{
	int i;
	NVDIMM_VDBG("\n***Flush Hint TBL %d Information***\n"
		"Socket ID: %u\n"
		"MemCtrl ID: %u\n"
		"Max Flush Hint Addrs: %u\n"
		"Valid Flush Addrs: %u\n",
		tbl_num,
		flush_tbl->socket_id,
		flush_tbl->mem_ctrl_id,
		flush_tbl->max_flush_hint_addrs,
		flush_tbl->num_valid_flush_hint_addrs
	);

	for (i = 0; i < flush_tbl->max_flush_hint_addrs; i++) {
		NVDIMM_VDBG("Flush Hint Addr#%d: %#lx", i,
			(unsigned long)flush_tbl->flush_hint_addr[i]);
	}
}

static void print_interleave_tbl(struct interleave_tbl *interleave,
	int interleave_tbl_num)
{
	int i;
	NVDIMM_VDBG("\n***Interleave TBL %d Information***\n"
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
		interleave->line_size
	);

	for (i = 0; i < interleave->lines_described; i++)
		NVDIMM_VDBG("Offset%d: %u", i, interleave->offsets[i]);

}

static void print_fit_header(struct fit_header *header)
{
	int i;
	struct nvdimm_fit *fit = header->fit;

	print_nvdimm_fit(fit);

	for (i = 0; i < fit->num_spa_range_tbl; i++)
		print_spa_rng_tbl(&header->spa_rng_tbls[i], i);


	for (i = 0; i < fit->num_memdev_spa_range_tbl; i++)
		print_memdev_spa_rng_tbl(&header->memdev_spa_rng_tbls[i], i);


	for (i = 0; i < fit->num_flush_hint_tbl; i++)
		print_flush_hint_tbl(&header->flush_tbls[i], i);


	for (i = 0; i < fit->num_interleave_tbl; i++)
		print_interleave_tbl(&header->interleave_tbls[i], i);

	NVDIMM_VDBG("Num DIMMs: %u\n" "Num PM DIMMs: %u", header->num_dimms,
		header->num_pm_dimms);
}

static int cmp_spa_rng_tbls(const void *a, const void *b)
{
	const struct spa_rng_tbl *tmp_a = (const struct spa_rng_tbl *) a;
	const struct spa_rng_tbl *tmp_b = (const struct spa_rng_tbl *) b;

	return tmp_a->spa_index - tmp_b->spa_index;
}

static int cmp_interleave_tbls(const void *a, const void *b)
{
	const struct interleave_tbl *tmp_a = (const struct interleave_tbl *) a;
	const struct interleave_tbl *tmp_b = (const struct interleave_tbl *) b;

	return tmp_a->interleave_idx - tmp_b->interleave_idx;
}

static struct spa_rng_tbl *create_spa_rng_tbls(__u16 num_spa_range_tbls,
	void *nvdimm_fit_ptr, int *cur_offset)
{
	int i;

	struct spa_rng_tbl *tables = kcalloc(num_spa_range_tbls,
		sizeof(*tables), GFP_KERNEL);
	if (!tables)
		return ERR_PTR(-ENOMEM);

	for (i = 0; i < num_spa_range_tbls; i++) {
		tables[i] =
			*(struct spa_rng_tbl *)
			(nvdimm_fit_ptr + *cur_offset);
		*cur_offset += sizeof(struct spa_rng_tbl);
	}

	sort(tables, num_spa_range_tbls, sizeof(struct spa_rng_tbl),
		cmp_spa_rng_tbls, NULL);

	return tables;
}

static struct memdev_spa_rng_tbl *create_memdev_spa_rng_tbls(
	__u16 num_memdev_spa_tbls, void *nvdimm_fit_ptr, int *cur_offset)
{
	int i;
	struct memdev_spa_rng_tbl *tables = kcalloc(num_memdev_spa_tbls,
		sizeof(*tables), GFP_KERNEL);

	if (!tables)
		return ERR_PTR(-ENOMEM);

	for (i = 0; i < num_memdev_spa_tbls; i++) {
		tables[i] = *(struct memdev_spa_rng_tbl *)
			(nvdimm_fit_ptr + *cur_offset);
		*cur_offset += sizeof(struct memdev_spa_rng_tbl);
	}

	return tables;
}

static struct flush_hint_tbl *create_flush_hint_tbls(__u16 num_flush_tbls,
	void *nvdimm_fit_ptr, int *cur_offset)
{
	int i;
	struct flush_hint_tbl *tables = kcalloc(num_flush_tbls,
		sizeof(*tables), GFP_KERNEL);

	if (!tables)
		return ERR_PTR(-ENOMEM);

	for (i = 0; i < num_flush_tbls; i++) {
		int j;
		__u64 first_addr;

		tables[i] = *(struct flush_hint_tbl *)
			(nvdimm_fit_ptr + *cur_offset);

		*cur_offset += sizeof(struct flush_hint_tbl) - sizeof(__u64 *);
		first_addr = *(__u64 *) (nvdimm_fit_ptr
			+ *cur_offset);
		*cur_offset += sizeof(__u64);

		tables[i].flush_hint_addr =
				kcalloc(tables[i].max_flush_hint_addrs,
				sizeof(*tables[i].flush_hint_addr), GFP_KERNEL);

		if (!tables[i].flush_hint_addr)
			goto err_out;

		tables[i].flush_hint_addr[0] = first_addr;

		for (j = 1; j < tables[i].max_flush_hint_addrs; j++) {
			tables[i].flush_hint_addr[j] =
				*(__u64 *) (nvdimm_fit_ptr
					+ *cur_offset);
			*cur_offset += sizeof(__u64);
		}
	}

	return tables;
err_out:
	for (i = i - 1; i >= 0; i--)
		kfree(tables[i].flush_hint_addr);

	kfree(tables);
	return ERR_PTR(-ENOMEM);
}

static struct interleave_tbl *create_interleave_tbls(__u16 num_interleave_tbls,
	void *nvdimm_fit_ptr, int *cur_offset)
{
	int i;
	struct interleave_tbl *tables = kcalloc(num_interleave_tbls,
		sizeof(*tables), GFP_KERNEL);

	if (!tables)
		return ERR_PTR(-ENOMEM);

	for (i = 0; i < num_interleave_tbls; i++) {
		int j;
		__u32 first_offset;

		tables[i] = *(struct interleave_tbl *)
			(nvdimm_fit_ptr + *cur_offset);

		*cur_offset += sizeof(struct interleave_tbl) - sizeof(__u32 *);
		first_offset = *(__u32 *) (nvdimm_fit_ptr
			+ *cur_offset);
		*cur_offset += sizeof(__u32);

		tables[i].offsets = kcalloc(tables[i].lines_described,
				sizeof(*tables[i].offsets), GFP_KERNEL);

		if (!tables[i].offsets)
			goto err_out;

		tables[i].offsets[0] = first_offset;

		for (j = 1; j < tables[i].lines_described; j++) {
			tables[i].offsets[j] =
				*(__u32 *) (nvdimm_fit_ptr
					+ *cur_offset);
			*cur_offset += sizeof(__u32);
		}

	}

	sort(tables, num_interleave_tbls, sizeof(struct interleave_tbl),
		cmp_interleave_tbls, NULL);

	return tables;

err_out:
	for (i = i - 1; i >= 0; i--)
		kfree(tables[i].offsets);

	kfree(tables);
	return ERR_PTR(-ENOMEM);
}

/* TODO: Enable Checksum when we determine which algorithm to use */
static int checksum_fit(void *nvdimm_fit_ptr, __u8 checksum, __u32 length)
{
	return 0;
}

static void free_intereleave_tbls(struct fit_header *fit_head)
{
	int i;

	for (i = 0; i < fit_head->fit->num_interleave_tbl; i++)
		kfree(fit_head->interleave_tbls[i].offsets);

	kfree(fit_head->interleave_tbls);
}

static void free_flush_hint_tbls(struct fit_header *fit_head)
{
	int i;

	for (i = 0; i < fit_head->fit->num_flush_hint_tbl; i++)
		kfree(fit_head->flush_tbls[i].flush_hint_addr);

	kfree(fit_head->flush_tbls);
}

/**
 * create_fit_table() - Create the entire FIT table from a physical address
 * region retrieved from the ACPI NS.
 * @nvdimm_fit_ptr: Pointer to the region of the physical address space that
 * contains the FIT
 *
 * The FIT is contained in one contiguous region of the physical address space.
 * Parse out the region and create structures
 * Determine how many DIMMs are present in the system
 * Determine how many DIMMs with PM regions are present in the system
 * The entire fit_header structure should be populated when the function
 * returns
 *
 * Generic
 *
 * Returns: Fully populated fit header, or ERR_PTR
 */
struct fit_header *create_fit_table(void *nvdimm_fit_ptr)
{
	struct fit_header *header;
	int cur_offset = 0;

	void *ret;

	if (nvdimm_fit_ptr == NULL)
		return NULL;

	header = kzalloc(sizeof(*header), GFP_KERNEL);

	if (!header) {
		ret = ERR_PTR(-ENOMEM);
		goto out;
	}

	header->fit = kzalloc(sizeof(*header->fit), GFP_KERNEL);

	if (!header->fit) {
		ret = ERR_PTR(-ENOMEM);
		goto after_fit_header;
	}

	*(header->fit) = *(struct nvdimm_fit *) (nvdimm_fit_ptr);

	if (header->fit->signature != NVDIMM_SIG) {
		NVDIMM_DBG("NVDIMM FIT Signature does not match\n");
		NVDIMM_DBG("Expected Sig: %u, Actual Sig: %u", NVDIMM_SIG,
			header->fit->signature);
		ret = ERR_PTR(-ENOTTY);/* No idea what Error Code to return */
		goto after_fit;
	}

	if (checksum_fit(nvdimm_fit_ptr, header->fit->checksum,
		header->fit->length)) {
		ret = ERR_PTR(-EFAULT);/* No idea what error code to return */
		goto after_fit;
	}

	cur_offset += sizeof(struct nvdimm_fit);

	header->spa_rng_tbls = create_spa_rng_tbls(
		header->fit->num_spa_range_tbl, nvdimm_fit_ptr, &cur_offset);

	if (IS_ERR(header->spa_rng_tbls)) {
		ret = header->spa_rng_tbls;
		goto after_fit;
	}

	header->memdev_spa_rng_tbls = create_memdev_spa_rng_tbls(
		header->fit->num_memdev_spa_range_tbl, nvdimm_fit_ptr,
		&cur_offset);

	if (IS_ERR(header->memdev_spa_rng_tbls)) {
		ret = header->memdev_spa_rng_tbls;
		goto after_spa_tables;
	}
	header->flush_tbls = create_flush_hint_tbls(
		header->fit->num_flush_hint_tbl, nvdimm_fit_ptr, &cur_offset);

	if (IS_ERR(header->flush_tbls)) {
		ret = header->flush_tbls;
		goto after_memdev_tbls;
	}

	header->interleave_tbls = create_interleave_tbls(
		header->fit->num_interleave_tbl, nvdimm_fit_ptr, &cur_offset);

	if (IS_ERR(header->interleave_tbls)) {
		ret = header->interleave_tbls;
		goto after_flush_tbls;
	}

	header->num_dimms = get_num_dimms(header->memdev_spa_rng_tbls,
		header->fit->num_memdev_spa_range_tbl);

	header->num_pm_dimms = get_num_pm_dimms(header);

	print_fit_header(header);

	return header;

after_flush_tbls:
	free_flush_hint_tbls(header);
after_memdev_tbls:
	kfree(header->memdev_spa_rng_tbls);
after_spa_tables:
	kfree(header->spa_rng_tbls);
after_fit:
	kfree(header->fit);
after_fit_header:
	kfree(header);
out:
	return ret;
}

void free_fit_table(struct fit_header *fit_head)
{
	if (!fit_head)
		return;
	free_intereleave_tbls(fit_head);
	free_flush_hint_tbls(fit_head);
	kfree(fit_head->memdev_spa_rng_tbls);
	kfree(fit_head->spa_rng_tbls);
	kfree(fit_head->fit);
	kfree(fit_head);
}
