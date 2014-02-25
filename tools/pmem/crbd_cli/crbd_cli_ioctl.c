#include <sys/ioctl.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>

#include <linux/nvdimm_ioctl.h>
#include <linux/cr_ioctl.h>

#include "acpi_config.h"
#include "crbd_cli_ioctl.h"

int check_root()
{
	if (getuid() != 0) {
		fprintf(stderr, "CRBD CLI must be ran as root.\n");
		return 1;
	}
	return 0;
}

int crbd_ioctl_get_topology(__u8 count, struct nvdimm_topology *dimm_topo)
{
	int ret = 0;
	int fd = 0;
	char buff[100];
	struct nvdimm_req nvdr;

	if(check_root())
		return -EPERM;

	snprintf(buff, sizeof(buff), "/dev/%s", CTRL_NAME);
	fd = open(buff, 0);

	if (fd < 0) {
		fprintf(stderr, "Unable to open ioctl target\n");
		return -ENOENT;
	}

	memset(&nvdr, 0, sizeof(nvdr));

	nvdr.nvdr_data_size = count * sizeof(*dimm_topo);
	nvdr.nvdr_data = dimm_topo;

	if (ioctl(fd, NVDIMM_GET_DIMM_TOPOLOGY, &nvdr)) {
		ret = -errno;
		fprintf(stderr, "Get Topology failed: %d\n", ret);
	}

	close(fd);

	return ret;
}

int crbd_ioctl_dimm_init()
{
	int fd = 0;
	char buff[100];
	int ret;

	if(check_root())
		return -EPERM;

	snprintf(buff, sizeof(buff), "/dev/%s", CTRL_NAME);
	fd = open(buff, 0);

	if (fd < 0) {
		fprintf(stderr, "Unable to open ioctl target\n");
		return -ENOENT;
	}

	if (ioctl(fd, NVDIMM_INIT)) {
		ret = -errno;
		close(fd);
		fprintf(stderr, "Error Initializing DIMM Inventory\n");
		if(ret == -EINVAL)
			fprintf(stderr, "Did you remember to load the FIT first?\n");
		return ret;
	}

	close(fd);

	return 0;
}

int crbd_ioctl_load_fit(char *nvdimm_fit_file)
{
	int fd = 0;
	void *nvdimm_fit_ptr;
	char buff[100];
	int ret;

	if(check_root())
		return -EPERM;

	nvdimm_fit_ptr = read_nvdimm_fit_file(nvdimm_fit_file);

	if (!nvdimm_fit_ptr) {
		fprintf(stderr, "Failed to Read NVDIMM FIT file\n");
		return -EINVAL;
	}

	snprintf(buff, sizeof(buff), "/dev/%s", CTRL_NAME);
	fd = open(buff, 0);

	if (fd < 0) {
		fprintf(stderr, "Unable to open ioctl target\n");
		return -ENOENT;
	}

	if (ioctl(fd, NVDIMM_LOAD_ACPI_FIT, nvdimm_fit_ptr)) {
		ret = -errno;
		close(fd);
		fprintf(stderr, "Error Loading ACPI FIT\n");
		return ret;
	}

	close(fd);

	return 0;
}

int crbd_identify_dimm(int dimm_handle)
{
	struct fv_fw_cmd fw_cmd;
	struct cr_pt_payload_identify_dimm dimm_id_payload;
	int ret = 0;
	__u64 raw_capacity_gb;

	memset(&fw_cmd, 0, sizeof(fw_cmd));

	fw_cmd.id = dimm_handle;
	fw_cmd.opcode = CR_PT_IDENTIFY_DIMM;
	fw_cmd.sub_opcode = 0;
	fw_cmd.input_payload_size = 0;
	fw_cmd.large_input_payload_size = 0;
	fw_cmd.output_payload_size = 128;
	fw_cmd.large_output_payload_size = 0;

	fw_cmd.output_payload = calloc(1, fw_cmd.output_payload_size);

	if(!fw_cmd.output_payload)
		return -ENOMEM;

	ret = crbd_ioctl_pass_thru(&fw_cmd);

	if (ret) {
		free(fw_cmd.output_payload);
		return ret;
	}
	dimm_id_payload = *(struct cr_pt_payload_identify_dimm *)fw_cmd.output_payload;

	free(fw_cmd.output_payload);

	raw_capacity_gb = dimm_id_payload.rc >> 18;

	fprintf(stdout, "Identify DIMM\n"
			"VendorID: %#hx\n"
			"DeviceID: %#hx\n"
			"RevisionID: %#hx\n"
			"InterfaceCode: %#hx\n"
			"FirmwareRevision(BCD): %c%c%c%c%c\n"
			"API Version(BCD): %#hhx\n"
			"Feature SW Mask: %#hhx\n"
			"NumberBlockWindows: %#hx\n"
			"NumberWFAs: %#hhx\n"
			"BlockCTRL_Offset: %#x\n"
			"RawCapacity: %llu GB\n"
			"Manufacturer: %s\n"
			"Serial Number: %s\n"
			"Model Number: %s\n",
			dimm_id_payload.vendor_id,
			dimm_id_payload.device_id,
			dimm_id_payload.revision_id,
			dimm_id_payload.ifc,
			dimm_id_payload.fwr[0],
			dimm_id_payload.fwr[1],
			dimm_id_payload.fwr[2],
			dimm_id_payload.fwr[3],
			dimm_id_payload.fwr[4],
			dimm_id_payload.api_ver,
			dimm_id_payload.fswr,
			dimm_id_payload.nbw,
			dimm_id_payload.nwfa,
			dimm_id_payload.obmcr,
			raw_capacity_gb,
			(char *)dimm_id_payload.mf,
			(char *)dimm_id_payload.sn,
			(char *)dimm_id_payload.mn);

	return ret;
}

int crbd_get_security(int dimm_handle, char *security_status)
{
	struct fv_fw_cmd fw_cmd;
	struct cr_pt_payload_get_security_state security_payload;
	int ret = 0;

	memset(&fw_cmd, 0, sizeof(fw_cmd));

	fw_cmd.id = dimm_handle;
	fw_cmd.opcode = CR_PT_GET_SEC_INFO;
	fw_cmd.sub_opcode = SUBOP_GET_SEC_STATE;
	fw_cmd.input_payload_size = 0;
	fw_cmd.large_input_payload_size = 0;
	fw_cmd.output_payload_size = 128;
	fw_cmd.large_output_payload_size = 0;

	fw_cmd.output_payload = calloc(1, fw_cmd.output_payload_size);

	if(!fw_cmd.output_payload)
		return -ENOMEM;

	ret = crbd_ioctl_pass_thru(&fw_cmd);

	if (ret) {
		free(fw_cmd.output_payload);
		return ret;
	}

	security_payload = *(struct cr_pt_payload_get_security_state *)fw_cmd.output_payload;

	free(fw_cmd.output_payload);

	fprintf(stdout, "Security State: %#hhx\n",security_payload.security_status);
	*security_status = security_payload.security_status;
	return ret;
}

int crbd_set_nonce(int dimm_handle)
{
	struct fv_fw_cmd fw_cmd;
	struct cr_pt_payload_smm_security_nonce security_nonce;
	int ret = 0;
	unsigned long long sample_nonce = 0x123456789ABCEF01;
	memset(&fw_cmd, 0, sizeof(fw_cmd));

	fw_cmd.id = dimm_handle;
	fw_cmd.opcode = CR_PT_SET_SEC_INFO;
	fw_cmd.sub_opcode = SUBOP_SET_NONCE;
	fw_cmd.input_payload_size = 8;
	fw_cmd.large_input_payload_size = 0;
	fw_cmd.output_payload_size = 0;
	fw_cmd.large_output_payload_size = 0;

	memcpy(security_nonce.sec_nonce, &sample_nonce, sizeof(unsigned long long));

	fw_cmd.input_payload = &security_nonce;

	ret = crbd_ioctl_pass_thru(&fw_cmd);

	return ret;
}

int crbd_set_passphrase(int dimm_handle, char *curr_ph, char *new_ph)
{
	struct fv_fw_cmd fw_cmd;
	struct cr_pt_payload_set_passphrase payload_ph;
	int ret = 0;

	memset(&fw_cmd, 0, sizeof(fw_cmd));

	fw_cmd.id = dimm_handle;
	fw_cmd.opcode = CR_PT_SET_SEC_INFO;
	fw_cmd.sub_opcode = SUBOP_SET_PASS;
	fw_cmd.input_payload_size = 64;
	fw_cmd.large_input_payload_size = 0;
	fw_cmd.output_payload_size = 0;
	fw_cmd.large_output_payload_size = 0;

	memcpy(payload_ph.passphrase_current, curr_ph, CR_PASSPHRASE_LEN);
	memcpy(payload_ph.passphrase_new, new_ph, CR_PASSPHRASE_LEN);

	fw_cmd.input_payload = &payload_ph;

	ret = crbd_ioctl_pass_thru(&fw_cmd);

	return ret;
}

int crbd_disable_passphrase(int dimm_handle, char *curr_ph)
{
	struct fv_fw_cmd fw_cmd;
	struct cr_pt_payload_passphrase payload_ph;
	int ret = 0;

	memset(&fw_cmd, 0, sizeof(fw_cmd));

	fw_cmd.id = dimm_handle;
	fw_cmd.opcode = CR_PT_SET_SEC_INFO;
	fw_cmd.sub_opcode = SUBOP_DISABLE_PASS;
	fw_cmd.input_payload_size = 32;
	fw_cmd.large_input_payload_size = 0;
	fw_cmd.output_payload_size = 0;
	fw_cmd.large_output_payload_size = 0;

	memcpy(payload_ph.passphrase_current, curr_ph, CR_PASSPHRASE_LEN);

	fw_cmd.input_payload = &payload_ph;

	ret = crbd_ioctl_pass_thru(&fw_cmd);

	return ret;
}

int crbd_unlock_unit(int dimm_handle, char *curr_ph)
{
	struct fv_fw_cmd fw_cmd;
	struct cr_pt_payload_passphrase payload_ph;
	int ret = 0;

	memset(&fw_cmd, 0, sizeof(fw_cmd));

	fw_cmd.id = dimm_handle;
	fw_cmd.opcode = CR_PT_SET_SEC_INFO;
	fw_cmd.sub_opcode = SUBOP_UNLOCK_UNIT;
	fw_cmd.input_payload_size = 32;
	fw_cmd.large_input_payload_size = 0;
	fw_cmd.output_payload_size = 0;
	fw_cmd.large_output_payload_size = 0;

	memcpy(payload_ph.passphrase_current, curr_ph, CR_PASSPHRASE_LEN);

	fw_cmd.input_payload = &payload_ph;

	ret = crbd_ioctl_pass_thru(&fw_cmd);

	return ret;
}

int crbd_erase_prepare(int dimm_handle)
{
	struct fv_fw_cmd fw_cmd;
	int ret = 0;

	memset(&fw_cmd, 0, sizeof(fw_cmd));

	fw_cmd.id = dimm_handle;
	fw_cmd.opcode = CR_PT_SET_SEC_INFO;
	fw_cmd.sub_opcode = SUBOP_SEC_ERASE_PREP;
	fw_cmd.input_payload_size = 0;
	fw_cmd.large_input_payload_size = 0;
	fw_cmd.output_payload_size = 0;
	fw_cmd.large_output_payload_size = 0;

	ret = crbd_ioctl_pass_thru(&fw_cmd);

	return ret;
}

int crbd_erase_unit(int dimm_handle, char *curr_ph)
{
	struct fv_fw_cmd fw_cmd;
	struct cr_pt_payload_passphrase payload_ph;
	int ret = 0;

	memset(&fw_cmd, 0, sizeof(fw_cmd));

	fw_cmd.id = dimm_handle;
	fw_cmd.opcode = CR_PT_SET_SEC_INFO;
	fw_cmd.sub_opcode = SUBOP_SEC_ERASE_UNIT;
	fw_cmd.input_payload_size = 32;
	fw_cmd.large_input_payload_size = 0;
	fw_cmd.output_payload_size = 0;
	fw_cmd.large_output_payload_size = 0;

	memcpy(payload_ph.passphrase_current, curr_ph, CR_PASSPHRASE_LEN);

	fw_cmd.input_payload = &payload_ph;

	ret = crbd_ioctl_pass_thru(&fw_cmd);

	return ret;
}

int crbd_freeze_lock(int dimm_handle)
{
	struct fv_fw_cmd fw_cmd;
	int ret = 0;

	memset(&fw_cmd, 0, sizeof(fw_cmd));

	fw_cmd.id = dimm_handle;
	fw_cmd.opcode = CR_PT_SET_SEC_INFO;
	fw_cmd.sub_opcode = SUBOP_SEC_FREEZE_LOCK;
	fw_cmd.input_payload_size = 0;
	fw_cmd.large_input_payload_size = 0;
	fw_cmd.output_payload_size = 0;
	fw_cmd.large_output_payload_size = 0;

	ret = crbd_ioctl_pass_thru(&fw_cmd);

	return ret;
}

int crbd_ioctl_pass_thru(struct fv_fw_cmd *fw_cmd)
{
	int ret = 0;
	int fd = 0;
	char buff[100];
	struct nvdimm_req nvdr;

	if(check_root())
		return -EPERM;

	snprintf(buff, sizeof(buff), "/dev/%s", CTRL_NAME);
	fd = open(buff, 0);

	if (fd < 0) {
		fprintf(stderr, "Unable to open ioctl target\n");
		return -ENOENT;
	}

	nvdr.nvdr_dimm_id = fw_cmd->id;
	nvdr.nvdr_data = fw_cmd;

	if ((ret = ioctl(fd, NVDIMM_PASSTHROUGH_CMD, &nvdr))) {
		if (ret < 0)
			ret = -errno;
		fprintf(stderr, "Pass_thru IOCTL failed: %d\n", ret);
	}

	close(fd);

	return ret;
}

int crbd_ioctl_topology_count(void)
{
	int dimm_count = 0;
	int fd = 0;
	char buff[100];

	if(check_root())
		return -EPERM;

	snprintf(buff, sizeof(buff), "/dev/%s", CTRL_NAME);
	fd = open(buff, 0);

	if (fd < 0) {
		fprintf(stderr, "Unable to open ioctl target\n");
		return -ENOENT;
	}

	dimm_count = ioctl(fd, NVDIMM_GET_TOPOLOGY_COUNT);

	if (dimm_count < 0) {
		dimm_count = -errno;
		fprintf(stderr, "Get NVDIMM Count failed\n");
	}

	close(fd);

	return dimm_count;
}
