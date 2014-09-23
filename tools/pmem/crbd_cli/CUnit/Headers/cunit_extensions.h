/*!
 * @file
 * @brief
 * This file contains the cunit extension MACROS and functions to make unit testing a little
 * easier.
 *
 * @details
 *
 * @internal
 * @copyright
 * Copyright 2012 Intel Corporation All Rights Reserved.
 *
 * INTEL CONFIDENTIAL
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
 * @endinternal
 */

#ifndef _CUNIT_EXTENSIONS_H_
#define _CUNIT_EXTENSIONS_H_

/*!
 * Macro that implements a CUnit test for equality for input arguments of type @b int
 * @remarks
 *		Indication of the failure will be output to console.
 * @param[in] actual
 * 		The integer value to be compared to @c expected
 * @param[in] expected
 * 		The expected integer value
 */
#define	MY_ASSERT_EQUAL(actual, expected) \
	{int l_expected = expected; int l_actual = actual; \
	MY_ASSERT_BASE(actual, expected, l_actual, l_expected, %d, CU_FALSE) }

/*!
 * Macro that implements a CUnit test for equality for input arguments of type @b int
 * @remarks
 *		Indication of the failure will be output to console.
 * @param[in] actual
 * 		The integer value to be compared to @c expected
 * @param[in] expected
 * 		The expected integer value
 */
#define	MY_ASSERT_EQUAL_PTR(actual, expected) \
	{void *l_expected = expected; void *l_actual = actual; \
	MY_ASSERT_BASE(actual, expected, l_actual, l_expected, %p, CU_FALSE) }

/*!
 * Macro that implements a CUnit test for equality for input arguments of type @b int
 * @remarks
 *		Indication of the failure will be output to console.
 * @note
 *		If @c actual is not equal to @c expected, the CUnit test being executed will fail and abort.
 * @param[in] actual
 * 		The integer value to be compared to @c expected
 * @param[in] expected
 * 		The expected integer value
 */
#define	MY_ASSERT_EQUAL_FATAL(actual, expected) \
	{int l_expected = expected; int l_actual = actual; \
	MY_ASSERT_BASE(actual, expected, l_actual, l_expected, %d, CU_TRUE) }

#ifdef __LINUX__
/*!
 * Macro that implements a CUnit test for equality for input arguments of type @b NVMD_UINT64
 * @remarks
 *		Indication of the failure will be output to console.
 * @param[in] actual
 * 		The integer value to be compared to @c expected
 * @param[in] expected
 * 		The expected integer value
 */
#define	MY_ASSERT_EQUAL64(actual, expected) \
	{ unsigned long long l_expected = (expected); unsigned long long l_actual = (actual); \
	MY_ASSERT_BASE((unsigned long long)(actual), (unsigned long long)(expected), \
			 l_actual, l_expected, 0x%llx, CU_FALSE) }
#else
/*!
 * Macro that implements a CUnit test for equality for input arguments of type @b NVMD_UINT64
 * @remarks
 *		Indication of the failure will be output to console.
 * @param[in] actual
 * 		The integer value to be compared to @c expected
 * @param[in] expected
 * 		The expected integer value
 */
#define	MY_ASSERT_EQUAL64(actual, expected) \
	{ unsigned long long l_expected = (expected); unsigned long long l_actual = (actual); \
	MY_ASSERT_BASE((unsigned long long)(actual), (unsigned long long)(expected), \
			l_actual, l_expected, 0x%I64x, CU_FALSE) }
#endif

/*!
 * Helper macro that implements a CUnit test for equality for two input arguments.
 * @remarks
 *		This macro should not be directly implemented in any test suites.
 * @param[in] actual
 * 		The integer value to be compared to @c expected
 * @param[in] expected
 * 		The expected integer value
 * @param[in] actual_value
 *		The value to format and display to console upon inequality.
 * @param[in] expected_value
 * 		The value to format and display to console upon inequality.
 * @param[in] format
 * 		A pattern to indicate the format for displaying the inputed values to the console.
 * @param[in] fatal
 * 		@b CU_FALSE = do not abort upon inequality, @b CU_TRUE = abort upon inequality
 */
#define MY_ASSERT_BASE(actual, expected, actual_value, expected_value, format, fatal) \
		{ \
			CU_BOOL b_value = (actual_value) == (expected_value); \
			if (!b_value) \
			{ \
				printf("\n%s does not equal %s ...\n", #expected, #actual); \
				printf("[%s: %d]Expected " #format ", but found " #format "\n", \
						__FILE__, __LINE__, expected_value, actual_value);\
			}\
			CU_assertImplementation(b_value, __LINE__,  \
					("MY_ASSERT_EQUAL(" #actual "," #expected ")"), __FILE__, "", fatal); \
		}

/*!
 * Macro that implements a CUnit test for inequality for input arguments of type @b int
 * @remarks
 *		Indication of the failure will be output to console.
 * @param[in] largerint
 * 		The larger integer value expected to be greater than but not equal to @c smallerint
 * @param[in] smallerint
 * 		The smaller integer value
 */
#define MY_ASSERT_GT(largerint, smallerint) \
		{ \
			int l_largerint = largerint; int l_smallerint = smallerint; \
			CU_BOOL b_value = (largerint) > (smallerint); \
			if (!b_value) \
			{ \
				printf("\n%s is not greater than %s ...\n", #largerint, #smallerint); \
				printf("[%s: %d]Expected %d to be greater than %d\n", \
						__FILE__, __LINE__, l_largerint, l_smallerint); \
			} \
		}

/*!
 * Macro that implements a CUnit test for equality for input arguments of type @b char*
 * @remarks
 *		Indication of the failure will be output to console.
 * @param[in] actual
 * 		The string to be compared against @c expected
 * @param[in] expected
 * 		The expected string
 */
#define MY_ASSERT_STRING_EQUAL(actual, expected) \
		{ \
			int l_result = strcmp((const char*)(actual), (const char*)(expected)); \
			CU_assertImplementation(!l_result, __LINE__, \
					("MY_ASSERT_STRING_EQUAL(" #actual ","  #expected ")"), \
					__FILE__, "", CU_FALSE); \
			if (l_result != 0) \
			{ \
				printf("\n%s does not equal %s ...\n", #expected, #actual); \
				printf("[%s: %d]Expected \"%s\", but found \"%s\"\n", \
						__FILE__, __LINE__, expected, actual);\
			} \
		} \

#endif /* _CUNIT_EXTENSIONS_H_ */
