/**
 * INTEL CONFIDENTIAL
 * Copyright (c) 2012 Intel Corporation. All Rights Reserved.
 *
 * The source code contained or described herein and all documents related
 * to the source code ("Material") are owned by Intel Corporation or its
 * suppliers or licensors. Title to the Material remains with Intel Corporation
 * or its suppliers and licensors. The Material contains trade secrets and
 * proprietary and confidential information of Intel or its suppliers and
 * licensors. The Material is protected by worldwide copyright and trade secret
 * laws and treaty provisions. No part of the Material may be used, copied,
 * reproduced, modified, published, uploaded, posted, transmitted, distributed,
 * or disclosed in any way without Intel's prior express written permission.
 *
 * No license under any patent, copyright, trade secret or other intellectual
 * property right is granted to or conferred upon you by disclosure or delivery
 * of the Materials, either expressly, by implication, inducement, estoppel or
 * otherwise. Any license under such intellectual property rights must be
 * express and approved by Intel in writing.
 */

package bist;

/**
 * Imports
 */
import com.intel.langutil.ArrayUtils;
import com.intel.util.IntelApplet;

/**
 * The purpose of this applet is to allow platform manufacturers to perform simple and
 * fast touch test to the Intel(r) ME-DAL feature
 * The applet can be run in two modes:
 * 1. COPY_COMPARE_BUFFER_TEST_CID - No user interaction (no input is needed except the command ID).
 * 2. ECHO_TEST_CID - Returns the input buffer to the user (buffer size is limited to 1-256 bytes).
 */
public class Bist extends IntelApplet
{
	/**
	* Static Final Members
	*/
	private static final int SUCCESS   = 0;
	private static final int FAILURE   = 1;

	private static final int COPY_COMPARE_BUFFER_TEST_CID = 0;
	private static final int ECHO_TEST_CID                = 1;
	private static final int WRONG_CID                    = 2;

	private static final int CONST_NUMBER     = 8;
	private static final int ARRAY_LENGTH     = 32;
	private static final int MAX_INPUT_LENGTH = 256;

	private static final byte[] SUCCESS_BUFFER = new byte[] {'S','U','C','C','E','S','S'};
	private static final byte[] FAILURE_BUFFER = new byte[] {'F','A','I','L','U','R','E'};

	/**
	 * Entry point of the applet.
	 * COPY_COMPARE_BUFFER_TEST_CID - Runs a simple internal test and returns its outcome
	 * ECHO_TEST_CID - If the given buffer is valid it returns the buffer as output (echo)
	 * Returns failure on all other command IDs
	 *
	 * @param commandID - The command to be executed
	 * @param input - The input buffer
	 * @return SUCCESS or FAILURE according to the relevant command execution
	 */
	public int invokeCommand(int commandID, byte[] input) {

		switch (commandID) {

		case COPY_COMPARE_BUFFER_TEST_CID:
			int res = copyCompareBufferTest();
			if (res == SUCCESS)
				setResponse(SUCCESS_BUFFER, 0, SUCCESS_BUFFER.length);
			else
				setResponse(FAILURE_BUFFER, 0, FAILURE_BUFFER.length);
			return res;

		case ECHO_TEST_CID:
			if ((input == null) || (input.length <= 0) || (input.length > MAX_INPUT_LENGTH)) {
				setResponse(FAILURE_BUFFER, 0, FAILURE_BUFFER.length);
				return FAILURE;
			}
			setResponse(input, 0, input.length);
			return SUCCESS;

		default:
			setResponse(FAILURE_BUFFER, 0, FAILURE_BUFFER.length);
			return WRONG_CID;
		}
	}

	/**
	 * Simple test which copies an array and compares it to the original array
	 * @return SUCCESS if arrays are identical, FAILURE otherwise
	 */
	private int copyCompareBufferTest() {
		boolean res;
		try {
			byte[] buffer1 = new byte[ARRAY_LENGTH];
			byte[] buffer2 = new byte[ARRAY_LENGTH];
			ArrayUtils.fillByteArray(buffer1, 0, ARRAY_LENGTH, (byte) CONST_NUMBER);
			ArrayUtils.copyByteArray(buffer1, 0, buffer2, 0, ARRAY_LENGTH);
			res = ArrayUtils.compareByteArray(buffer1, 0, buffer2, 0, ARRAY_LENGTH);
		}
		catch (Exception e) {
			res = false;
		}
		if (res)
			return SUCCESS;
		return FAILURE;
	}
}
