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
 * integ_test_driver.c
 *
 * This file contains the integration test driver for CRBD with the linux kernel
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <assert.h>

#include <CUnit.h>
#include <Basic.h>

/*
 * actual tests are in other files --these methods are used to
 * load them into the suites
 */
extern int load_integration_tests(CU_pSuite suite);
extern int integration_test_setup();
extern int integration_test_teardown();

/*
 * Driver for the management library unit tests.
 */
int run_unit_tests(void)
{
	CU_pSuite integration_suite = NULL;

	unsigned int result;

	/* could make this caller settable */
	CU_basic_set_mode(CU_BRM_VERBOSE);

	/* initialize the CUnit framework */
	assert(CU_initialize_registry() == CUE_SUCCESS);

	/* create a test suite for the core features */
	integration_suite = CU_add_suite("Core Suite",
		integration_test_setup, integration_test_teardown);
	assert(integration_suite != NULL);
	assert(load_integration_tests(integration_suite) >= 0);

	CU_basic_run_tests();

	/*
	 * So if there no failures this program returns 0.  If there are
	 * test failures it returns the number of failures.  This works well
	 * in most shell environments and when called from make as 0 is taken
	 * to be a successful run and anything else represents an error.
	 */
	result = CU_get_number_of_failures();
	/* shutdown the CUnit test framework */
	CU_cleanup_registry();

	fflush(stdout);
	return result;
}
