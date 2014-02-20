#include <linux/limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <getopt.h>

#include <linux/nvdimm_ioctl.h>

#include "crbd_cli_ioctl.h"

extern int run_unit_tests(void);
extern char nvdimm_fit_file[PATH_MAX];

static const char usage_cmds[] =
"usage: %s commands\n"
"  --load_fit [NVDIMM FIT file | default]\n"
"  --init_dimms\n"
"  --unit_tests [NVDIMM FIT file]\n"
"  --topo_count\n"
"  --get_topology\n"
"  --id_dimm dimm_handle\n"
"  --help \n"
;

enum {
	LOAD_FIT,
	UNIT_TESTS,
	INIT_DIMMS,
	TOPO_COUNT,
	GET_TOPOLOGY,
	ID_DIMM,
};

static struct option long_options[] = {
	{"load_fit",	required_argument,	0,	LOAD_FIT},
	{"unit_tests",	required_argument,	0,	UNIT_TESTS},
	{"init_dimms",  no_argument,		0,	INIT_DIMMS},
	{"topo_count",	no_argument,		0,	TOPO_COUNT},
	{"get_topology", no_argument,		0,	GET_TOPOLOGY},
	{"id_dimm", required_argument,		0,	ID_DIMM},
	{0, 0, 0, 0}
};

void usage(const char *program)
{
	fprintf(stderr, usage_cmds, program);
}

static void print_dimm_topology(int num_dimms, struct nvdimm_topology *dimm_topo)
{
	int i;

	for (i = 0; i < num_dimms; i++) {
		printf("***NVDIMM %hu Topology***\n"
			"Vendor ID: %u\n"
			"Device ID: %hu\n"
			"Revision ID: %hu\n"
			"Format Interface Code: %hu\n"
			"Socket ID: %hu\n"
			"iMC ID: %hu\n"
			"Health: %hu\n",
			dimm_topo[i].id,
			dimm_topo[i].vendor_id,
			dimm_topo[i].device_id,
			dimm_topo[i].revision_id,
			dimm_topo[i].fmt_interface_code,
			dimm_topo[i].proximity_domain,
			dimm_topo[i].memory_controller_id,
			dimm_topo[i].health);
	}
}

static int get_topology(void)
{
	int num_dimms;
	struct nvdimm_topology *dimm_topo;
	int ret;

	num_dimms = crbd_ioctl_topology_count();

	if (num_dimms < 0) {
		fprintf(stderr, "Get NVDIMM topology count failed");
		return EXIT_FAILURE;
	}

	if (!num_dimms)
		return EXIT_SUCCESS;

	dimm_topo = calloc(num_dimms,sizeof(*dimm_topo));

	if (!dimm_topo)
		return EXIT_FAILURE;

	ret = crbd_ioctl_get_topology(num_dimms, dimm_topo);

	if (ret)
		return EXIT_FAILURE;

	print_dimm_topology(num_dimms, dimm_topo);

	free(dimm_topo);

	return EXIT_SUCCESS;
}

int main(int argc, char *argv[])
{
	nvdimm_fit_file[0] = '\0';

	while (1) {
		int option_index;
		int dimm_count;
		int ret;
		switch (getopt_long(argc, argv, "h", long_options,
			&option_index)) {
		case LOAD_FIT:
			if (strcmp(optarg, "default"))
				strncpy(nvdimm_fit_file, optarg, PATH_MAX);

			if (crbd_ioctl_load_fit(nvdimm_fit_file)) {
				fprintf(stderr, "Load NVDIMM FIT Failed\n");
				return EXIT_FAILURE;
			}
			return EXIT_SUCCESS;
			break;
		case UNIT_TESTS:
			if (strcmp(optarg, "default"))
				strncpy(nvdimm_fit_file, optarg, PATH_MAX);

			if (!run_unit_tests())
				return EXIT_FAILURE;

			return EXIT_SUCCESS;
			break;
		case INIT_DIMMS:
			if (crbd_ioctl_dimm_init()) {
				fprintf(stderr, "Initialize DIMM Inventory Failed\n");
				return EXIT_FAILURE;
			}
			return EXIT_SUCCESS;
			break;
		case TOPO_COUNT:
			dimm_count = crbd_ioctl_topology_count();
			if (dimm_count < 0) {
				fprintf(stderr, "Get NVDIMM topology count failed\n");
				return EXIT_FAILURE;
			}
			fprintf(stderr, "NVDIMM Count: %d\n",dimm_count);
			return EXIT_SUCCESS;
			break;
		case GET_TOPOLOGY:
			if (get_topology())
				return EXIT_FAILURE;
			return EXIT_SUCCESS;
			break;
		case ID_DIMM:
			if ((ret = crbd_identify_dimm(strtol(optarg, NULL, 0)))) {
				fprintf(stderr, "Identify DIMM failed: Error %d\n", ret);
				return EXIT_FAILURE;
			}
			return EXIT_SUCCESS;
			break;
		default:
			usage(argv[0]);
			exit(EXIT_FAILURE);
		}
	}
}
