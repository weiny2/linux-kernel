/*
 * The interface between the platform independent device adapter and the device
 * driver, that defines the shared structures shared across that interface.
 *
 * Copyright 2013-2014 Intel Corporation All Rights Reserved.
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
 * Unless otherwise agreed by Intel in writing, you may not remove or alter this
 * notice or any other notice embedded in Materials by Intel or Intel's
 * suppliers or licensors in any way.
 */

#ifndef	_NVDIMM_H
#define	_NVDIMM_H

#define	MAX_REGIONS_PER_VOLUME	60 /* Maximum regions per volume */

#define	APPNAME	"NVDIMM" /* App name */
#define	LEVEL	APPNAME /* Level */

/* Maximum number of DIMMs possible for a given memory topology */
#define	NVDIMM_MAX_TOPO_SIZE	96
/* Maximum number of DIMMs possible for an memory NUMA interleave */
#define	MAX_INTERLEAVE	12
/* UUID length */
#define	NVDIMM_UUID_LEN	16
/* Part number length */
#define	NVDIMM_PART_NUMBER_LEN	32
/* Device locator length */
#define	NVDIMM_DEVICE_LOCATOR_LEN	128
/* Bank label length */
#define	NVDIMM_BANK_LABEL_LEN	128
/* Volume 'Friendly Name' buffer length */
#define	NVDIMM_VOL_FRIENDLY_NAME_LEN	32
/* Volume 'Purpose' buffer length */
#define	NVDIMM_VOL_PURPOSE_LEN	100
/* Driver revision length */
#define	NVDIMM_DRIVER_REV_LEN	16

#define CTRL_NAME "pmem" /* Linux device name */

enum nvdimm_format {
	NO_FORMAT = 0,
	AEP_DIMM = 1,
};

enum nvdimm_health {
	NVDIMM_UNINITIALIZED = 0,
	NVDIMM_INITIALIZED = 1,
};

enum pool_attributes {
	/* Pool memory supports persistent memory access */
	POOL_PM_CAPABLE = (1 << 2),
	POOL_DPA = (1 << 6), /* Pool memory expects DPAs for block I/O */
	POOL_SPA = (1 << 7), /* Pool memory expects SPAs for block I/O */
	/* TODO: Cached needs more clarification */
	POOL_CACHED = (1 << 16), /* Pool is being used for file caching */
	POOL_MIRRORED = (1 << 17), /* Pool memory is being mirrored by HW */
	POOL_ENCRYPTED = (1 << 18), /* Pool memory is encrypted by HW */
};

enum volume_attributes {
	VOLUME_ENABLED = (1 << 0), /* Volume has been enabled by user */
	/* Volume is not missing any extents/labels */
	VOLUME_COMPLETE = (1 << 1),
	/* Volume supports persistent memory access */
	VOLUME_PM_CAPABLE = (1 << 2),
	VOLUME_DPA = (1 << 6), /* LBAs are converted to DPAs */
	VOLUME_SPA = (1 << 7), /* LBAs are converted to SPAs */
	/* Volume enforces single sector power fail atomicity */
	VOLUME_ATOMIC = (1 << 8),
	/* Atomicity is supported through the use of a BTT and freelist */
	VOLUME_BTT_ATOMIC = (1 << 9),
};

#endif /* _NVDIMM_H */
