#include <sys/ioctl.h>
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

	if (ioctl(fd, NVDIMM_PASSTHROUGH_CMD, &nvdr)) {
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
