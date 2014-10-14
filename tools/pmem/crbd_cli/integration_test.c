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

#include <linux/byteorder/little_endian.h>

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

void test_id_dimm(void)
{
	int test_dimm = 1;
	int ret = 0;
	struct fv_fw_cmd fw_cmd;
	struct cr_pt_payload_identify_dimm id_dimm_payload;

	fw_cmd.id = test_dimm;
	fw_cmd.opcode = CR_PT_IDENTIFY_DIMM;
	fw_cmd.sub_opcode = 0x00;
	fw_cmd.input_payload_size = 0;
	fw_cmd.large_input_payload_size = 0;
	fw_cmd.output_payload_size = 128;
	fw_cmd.large_output_payload_size = 0;

	fw_cmd.output_payload = calloc(1, fw_cmd.output_payload_size);
	CU_ASSERT_FATAL(!fw_cmd.output_payload == 0);

	ret = crbd_ioctl_pass_thru(&fw_cmd);

	MY_ASSERT_EQUAL(ret, FNV_MB_SUCCESS);

	id_dimm_payload = *(struct cr_pt_payload_identify_dimm *) fw_cmd.output_payload;

	free(fw_cmd.output_payload);

	MY_ASSERT_EQUAL(__le16_to_cpu(id_dimm_payload.vendor_id), 0x8086);
	MY_ASSERT_EQUAL(__le16_to_cpu(id_dimm_payload.device_id), 0x2017);
	MY_ASSERT_EQUAL(__le16_to_cpu(id_dimm_payload.revision_id), 0x00);
	MY_ASSERT_EQUAL(__le16_to_cpu(id_dimm_payload.ifc), AEP_DIMM);
	MY_ASSERT_EQUAL(id_dimm_payload.api_ver, 0x12);
	MY_ASSERT_EQUAL(id_dimm_payload.fswr, 0x00);
	MY_ASSERT_EQUAL(__le16_to_cpu(id_dimm_payload.nbw), 0x00);
	MY_ASSERT_EQUAL(__le16_to_cpu(id_dimm_payload.nwfa), 0x00);
	MY_ASSERT_EQUAL(__le16_to_cpu(id_dimm_payload.obmcr), 0x00);
	MY_ASSERT_EQUAL(__le32_to_cpu(id_dimm_payload.rc), 0x400000);
}

void test_get_security(void)
{
	int test_dimm = 1;
	char security_status;

	MY_ASSERT_EQUAL(crbd_get_security(test_dimm, &security_status)
			, FNV_MB_SUCCESS);
	MY_ASSERT_EQUAL(security_status, 0);
}

void test_set_nonce(void)
{
	int test_dimm = 1;
	MY_ASSERT_EQUAL(crbd_set_nonce(test_dimm), FNV_MB_UNSUPPORTED_CMD);
}

void test_passphrase(void)
{
	int test_dimm = 1;
	char pphrase[CR_PASSPHRASE_LEN + 1];
	char empty_pphrase[CR_PASSPHRASE_LEN + 1];
	char security_status;

	MY_ASSERT_EQUAL(crbd_get_security(test_dimm, &security_status)
			, FNV_MB_SUCCESS);
	MY_ASSERT_EQUAL(security_status, 0);

	strncpy(pphrase, "monkey", sizeof(pphrase));
	memset(empty_pphrase, 0, sizeof(empty_pphrase));

	/*Set Passphrase*/
	MY_ASSERT_EQUAL(crbd_set_passphrase(test_dimm, empty_pphrase, pphrase),
			FNV_MB_SUCCESS);

	/*Verify Security State*/
	MY_ASSERT_EQUAL(crbd_get_security(test_dimm, &security_status)
			, FNV_MB_SUCCESS);
	MY_ASSERT_EQUAL(security_status, CR_SEC_ENABLED);

	/*Disable Passphrase*/
	MY_ASSERT_EQUAL(crbd_disable_passphrase(test_dimm, pphrase)
			, FNV_MB_SUCCESS);

	/*Check Security State*/
	MY_ASSERT_EQUAL(crbd_get_security(test_dimm, &security_status)
			, FNV_MB_SUCCESS);
	MY_ASSERT_EQUAL(security_status, 0);
}

void test_secure_erase(void)
{
	int test_dimm = 1;
	char pphrase[CR_PASSPHRASE_LEN + 1];
	char empty_pphrase[CR_PASSPHRASE_LEN + 1];
	char security_status;

	MY_ASSERT_EQUAL(crbd_get_security(test_dimm, &security_status)
			, FNV_MB_SUCCESS);
	MY_ASSERT_EQUAL(security_status, 0);

	strncpy(pphrase, "monkey", sizeof(pphrase));
	memset(empty_pphrase, 0, sizeof(empty_pphrase));

	/*Set Passphrase*/
	MY_ASSERT_EQUAL(crbd_set_passphrase(test_dimm, empty_pphrase, pphrase),
			FNV_MB_SUCCESS);

	/*Verify Security State*/
	MY_ASSERT_EQUAL(crbd_get_security(test_dimm, &security_status)
			, FNV_MB_SUCCESS);
	MY_ASSERT_EQUAL(security_status, CR_SEC_ENABLED);

	/*Erase Prepare*/
	MY_ASSERT_EQUAL(crbd_erase_prepare(test_dimm)
				, FNV_MB_SUCCESS);

	/*Secure Erase*/
	MY_ASSERT_EQUAL(crbd_erase_unit(test_dimm, pphrase)
			, FNV_MB_SUCCESS);

	/*Check Security State*/
	MY_ASSERT_EQUAL(crbd_get_security(test_dimm, &security_status)
			, FNV_MB_SUCCESS);
	MY_ASSERT_EQUAL(security_status, 0);
}

void test_get_dimm_topology()
{
	int num_dimms;
	struct nvdimm_topology *dimm_topo;
	int ret;

	num_dimms = crbd_ioctl_topology_count();

	MY_ASSERT_EQUAL(num_dimms, 1);

	dimm_topo = calloc(num_dimms,sizeof(*dimm_topo));

	CU_ASSERT_FATAL(!dimm_topo == 0);

	ret = crbd_ioctl_get_topology(num_dimms, dimm_topo);

	MY_ASSERT_EQUAL(ret,0);

	MY_ASSERT_EQUAL(dimm_topo[0].id, 1);
	MY_ASSERT_EQUAL(dimm_topo[0].vendor_id, 0x8086);
	MY_ASSERT_EQUAL(dimm_topo[0].device_id, 0x2017);
	MY_ASSERT_EQUAL(dimm_topo[0].revision_id, 0);
	MY_ASSERT_EQUAL(dimm_topo[0].fmt_interface_code, AEP_DIMM);
	MY_ASSERT_EQUAL(dimm_topo[0].proximity_domain, 1);
	MY_ASSERT_EQUAL(dimm_topo[0].memory_controller_id, 0);

	ret = crbd_ioctl_get_topology(num_dimms - 1, dimm_topo);

	MY_ASSERT_EQUAL(ret, -EINVAL);

	free(dimm_topo);
}

void test_topology_count(void)
{
	int count;

	count = crbd_ioctl_topology_count();

	MY_ASSERT_EQUAL(count,1);
}

int load_integration_tests(CU_pSuite suite)
{
	int test_count = 0;

	assert(CU_add_test(suite, "cr_get_dimm_topology", test_get_dimm_topology) != NULL);
	test_count++;
	assert(CU_add_test(suite, "cr_id_dimm", test_id_dimm) != NULL);
	test_count++;
	assert(CU_add_test(suite, "cr_get_security", test_get_security) != NULL);
	test_count++;
	assert(CU_add_test(suite, "cr_set_nonce", test_set_nonce) != NULL);
	test_count++;
	assert(CU_add_test(suite, "cr_test_passphrase", test_passphrase) != NULL);
	test_count++;
	assert(CU_add_test(suite, "cr_test_secure_erase", test_secure_erase) != NULL);
	test_count++;
	assert(CU_add_test(suite, "cr_topology_count", test_topology_count) != NULL);
	test_count++;

	return test_count;
}
