/*
 * INTEL CONFIDENTIAL
 *
 * Copyright 2013 Intel Corporation All Rights Reserved.
 *
 * The source code contained or described herein and all documents related to the
 * source code ("Material") are owned by Intel Corporation or its suppliers or
 * licensors. Title to the Material remains with Intel Corporation or its
 * suppliers and licensors. The Material may contain trade secrets and proprietary
 * and confidential information of Intel Corporation and its suppliers and licensors,
 * and is protected by worldwide copyright and trade secret laws and treaty
 * provisions. No part of the Material may be used, copied, reproduced, modified,
 * published, uploaded, posted, transmitted, distributed, or disclosed in any way
 * without Intel's prior express written permission.
 *
 * No license under any patent, copyright, trade secret or other intellectual
 * property right is granted to or conferred upon you by disclosure or delivery
 * of the Materials, either expressly, by implication, inducement, estoppel or
 * otherwise. Any license under such intellectual property rights must be express
 * and approved by Intel in writing.
 *
 * Unless otherwise agreed by Intel in writing, you may not remove or alter this
 * notice or any other notice embedded in Materials by Intel or Intel's suppliers
 * or licensors in any way.
 *
 * This file contains the integration tests for the crbd ioctl interface
 *
 */

#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <sys/ioctl.h>
#include <linux/limits.h>

#include <CUnit.h>
#include <cunit_extensions.h>
#include <Basic.h>

#include <linux/nvdimm.h>
#include <linux/nvdimm_ioctl.h>
#include <linux/cr_ioctl.h>
#include <linux/nvdimm_acpi.h>

#include "acpi_config.h"
#include "crbd_cli_ioctl.h"

char nvdimm_fit_file[PATH_MAX];

/*
 * Setup
 * TODO: load simulation
 */
int integration_test_setup()
{
	int ret;

	ret = crbd_ioctl_load_fit(nvdimm_fit_file);

	if (ret != -EEXIST && ret)
		return -1;

	ret = crbd_ioctl_dimm_init();

	if (ret != -EEXIST && ret)
		return -1;

	return 0;
}

/*
 * Cleanup
 * TODO: unload simulation
 */
int integration_test_teardown()
{
	return 0;
}

void test_pass_thru(void)
{
	char testpayload[] = "AppleCandy";
	char large_testpayload[] = "CherryCandy";
	int test_dimm = 1;
	int ret = 0;
	struct fv_fw_cmd fw_cmd;

	fw_cmd.id = test_dimm;
	fw_cmd.opcode = CR_PT_IDENTIFY_DIMM;
	fw_cmd.sub_opcode = 0xFF;
	fw_cmd.input_payload_size = 128;
	fw_cmd.large_input_payload_size = (1 << 20);
	fw_cmd.output_payload_size = 128;
	fw_cmd.large_output_payload_size = (1 << 20);

	fw_cmd.input_payload = calloc(1, fw_cmd.input_payload_size);
	CU_ASSERT_FATAL(!fw_cmd.input_payload == 0);

	fw_cmd.large_input_payload = calloc(1, fw_cmd.large_input_payload_size);
	CU_ASSERT_FATAL(!fw_cmd.large_input_payload == 0);

	fw_cmd.output_payload = calloc(1, fw_cmd.output_payload_size);
	CU_ASSERT_FATAL(!fw_cmd.output_payload == 0);

	fw_cmd.large_output_payload = calloc(1, fw_cmd.large_output_payload_size);
	CU_ASSERT_FATAL(!fw_cmd.large_output_payload == 0);

	memcpy(fw_cmd.input_payload, testpayload, sizeof(testpayload));
	memcpy(fw_cmd.large_input_payload, large_testpayload, sizeof(large_testpayload));

	ret = crbd_ioctl_pass_thru(&fw_cmd);

	MY_ASSERT_EQUAL(ret, 0);

	MY_ASSERT_EQUAL(strcmp(testpayload, (char * )fw_cmd.output_payload), 0);
	MY_ASSERT_EQUAL(
			strcmp(large_testpayload,
				(char * )fw_cmd.large_output_payload), 0);
	free(fw_cmd.input_payload);
	free(fw_cmd.large_input_payload);
	
	fw_cmd.input_payload_size = 0;
	fw_cmd.large_input_payload_size = 0;
	
	ret = crbd_ioctl_pass_thru(&fw_cmd);

	MY_ASSERT_EQUAL(ret, 0);

	free(fw_cmd.output_payload);
	free(fw_cmd.large_output_payload);

	fw_cmd.output_payload_size = 0;
	fw_cmd.large_output_payload_size = 0;

	ret = crbd_ioctl_pass_thru(&fw_cmd);

	MY_ASSERT_EQUAL(ret, 0);

}

void test_get_dimm_topology()
{
	int num_dimms;
	struct nvdimm_topology *dimm_topo;
	int ret;

	num_dimms = crbd_ioctl_topology_count();

	MY_ASSERT_EQUAL(num_dimms, 6);

	dimm_topo = calloc(num_dimms,sizeof(*dimm_topo));

	CU_ASSERT_FATAL(!dimm_topo == 0);

	ret = crbd_ioctl_get_topology(num_dimms, dimm_topo);

	MY_ASSERT_EQUAL(ret,0);

	MY_ASSERT_EQUAL(dimm_topo[0].id, 1);
	MY_ASSERT_EQUAL(dimm_topo[0].vendor_id, 32896);
	MY_ASSERT_EQUAL(dimm_topo[0].device_id, 1234);
	MY_ASSERT_EQUAL(dimm_topo[0].revision_id, 0);
	MY_ASSERT_EQUAL(dimm_topo[0].fmt_interface_code, AEP_DIMM);
	MY_ASSERT_EQUAL(dimm_topo[0].proximity_domain, 1);
	MY_ASSERT_EQUAL(dimm_topo[0].memory_controller_id, 0);

	MY_ASSERT_EQUAL(dimm_topo[5].id, 6);
	MY_ASSERT_EQUAL(dimm_topo[5].vendor_id, 32896);
	MY_ASSERT_EQUAL(dimm_topo[5].device_id, 1234);
	MY_ASSERT_EQUAL(dimm_topo[5].revision_id, 0);
	MY_ASSERT_EQUAL(dimm_topo[5].fmt_interface_code, AEP_DIMM);
	MY_ASSERT_EQUAL(dimm_topo[5].proximity_domain, 1);
	MY_ASSERT_EQUAL(dimm_topo[5].memory_controller_id, 1);

	ret = crbd_ioctl_get_topology(num_dimms - 1, dimm_topo);

	MY_ASSERT_EQUAL(ret, -EINVAL);

	free(dimm_topo);
}

void test_topology_count(void)
{
	int count;

	count = crbd_ioctl_topology_count();

	MY_ASSERT_EQUAL(count,6);
}

int load_integration_tests(CU_pSuite suite)
{
	int test_count = 0;

	assert(CU_add_test(suite, "cr_get_dimm_topology", test_get_dimm_topology) != NULL);
	test_count++;
	assert(CU_add_test(suite, "cr_pass_thru", test_pass_thru) != NULL);
	test_count++;
	assert(CU_add_test(suite, "cr_topology_count", test_topology_count) != NULL);
	test_count++;

	return test_count;
}
