/*
 * The interface between the platform independent device adapter and the device
 * driver, that defines the shared structures shared across that interface.
 *
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
 * Unless otherwise agreed by Intel in writing, you may not remove or alter this
 * notice or any other notice embedded in Materials by Intel or Intel's
 * suppliers or licensors in any way.
 */

#ifndef	NVDIMM_IOCTL_H_
#define	NVDIMM_IOCTL_H_

#include <linux/nvdimm.h>

#ifdef __linux__
#include <linux/ioctl.h>
#endif

/*
 * ****************************************************************************
 * ENUMS
 * ****************************************************************************
 */

/*
 * Defines the complete set of IOCTLs that are supported by the Generic NVDIMM
 * Driver
 * @remarks
 *	Enumeration ranges are used to group IOCTLs by functionality
 */
enum nvdimm_ioctl_type {
	/* Get the platform capabilities from the NFIT */
	IOCTL_GET_PLATFORM_CAPABILITIES = 1,
	/* Get the number of dimms in the system's memory topology */
	IOCTL_GET_TOPOLOGY_COUNT = 11,
	IOCTL_GET_TOPOLOGY = 12, /* Get the system's memory topology */
	/*TODO Get the details for one specific Vendor DIMM */
	IOCTL_GET_DIMM_DETAILS = 13,

	IOCTL_GET_POOL_COUNT = 21, /* Get the number of pools allocated */
	IOCTL_GET_POOLS = 22, /* Get the details for a given number of pools */
	IOCTL_GET_POOL_DETAILS = 23, /* Get the details for a specific pool */

	IOCTL_GET_VOLUME_COUNT = 31, /* Get the number of existing volumes */
	/* Get the details for a given number of pools */
	IOCTL_GET_VOLUMES = 32,
	/* Get the details for a specific volume */
	IOCTL_GET_VOLUME_DETAILS = 33,

	IOCTL_CREATE_VOLUME = 131, /* Create a new volume */
	IOCTL_DELETE_VOLUME = 132, /* Delete a given volume */
	IOCTL_MODIFY_VOLUME = 133, /* Modify a given volume */

	IOCTL_PASSTHROUGH_CMD = 255 /* Execute a passthrough IOCTL */
};


/*
 * ****************************************************************************
 * STRUCTURES
 * ****************************************************************************
 */

/*
 * Describes the system-level view of a memory module's properties,
 * based upon its physical location and access patterns within the system
 */

struct nvdimm_topology {
	unsigned short id; /* The physical ID of the memory module */
	unsigned short vendor_id; /* The vendor identifier */
	unsigned short device_id; /* The device identifier */
	unsigned short revision_id; /* The revision identifier */
	unsigned char type; /* Type of memory used by this device */
	/* The current health of the NVDIMM
	 *  0 - Uninitialized - NVDIMM present in system but a driver has
	 *  not initialized the nvdimm
	 *  1 - Initialized - NVDIMM is ready to be used and is attached to
	 *  a vendor driver
	 *  2 - Error? - ???
	 */
	unsigned char health;
	unsigned short fmt_interface_code; /* The device type */
	/* The numa node. Equates to the ID of the socket */
	unsigned short proximity_domain;
	/* The ID of the associated memory controller */
	unsigned short memory_controller_id;
	/* ASCII revision of the driver */
	char driver_rev[NVDIMM_DRIVER_REV_LEN];
};

/*
 * Detailed information about a specific DIMM including information from the
 *  SMBIOS tables
 */
struct nvdimm_details {
	/* SMBIOS Type 17 Table Info */
	unsigned short form_factor; /* DIMM Form Factor */
	/* Width in bits used to store user data */
	unsigned long long data_width;
	unsigned long long total_width; /* Width in bits for data and ECC */
	unsigned long long speed; /* Speed in MHz */
	char part_number[NVDIMM_PART_NUMBER_LEN]; /* DIMM part number */
	char device_locator[NVDIMM_DEVICE_LOCATOR_LEN]; /*Socket or board pos*/
};

/*
 * A lightweight description of a pool.
 */
struct nvdimm_pool_discovery {
	unsigned short id; /* The unique system ID of the pool */
};

/*
 * Describes the features pertaining to the unique construction and usage of a
 * pool of memory.
 */
struct nvdimm_pool_details {
	/* The pool discovery information */
	struct nvdimm_pool_discovery discovery;

	/*
	 * Number of NVDIMMs present in the memory interleave
	 * for this pool. Also equal to the number of NVDIMMs
	 * present in the pool
	 */
	unsigned char interleave_way;
	unsigned int interleave_size;
	unsigned short proximity_domain;

	/*
	 * Bitwise QoS attributes for the pool
	 */
	unsigned int pool_attributes;
	unsigned long long capacity; /* The capacity of the pool, in MB */

	/*
	 * A list of the DIMMs used to implement the pool,
	 * allocating for max DIMMs possible
	 */
	unsigned short dimm_id[NVDIMM_MAX_TOPO_SIZE];
};

/*
 * A lightweight description of a volume.
 */
struct nvdimm_volume_discovery {
	/* The unique ID of the volume */
	unsigned char uuid[NVDIMM_UUID_LEN];
	/* Human assigned name of the volume */
	char friendly_name[NVDIMM_VOL_FRIENDLY_NAME_LEN];
};

/*
 * Describes the features that detail a volume's construction.
 */
struct nvdimm_volume_details {
	 /* The volume discovery information */
	struct nvdimm_volume_discovery discovery;

	char purpose[NVDIMM_VOL_PURPOSE_LEN];
	unsigned long long creation_date; /* Seconds since epoch. */

	unsigned long long block_size; /* The size of a block in bytes */
	unsigned long long block_count; /* The number of blocks in the volume */
	 /* The number of pools this volume is constructed from */
	unsigned int pool_count;
	/*
	 * Bitwise QoS attributes for the volume
	 */
	unsigned int volume_attributes;
	/* the rolled-up health of the underlying pools/dimms */
	unsigned char health;

	/*
	 * The pools that the volume is comprised of
	 */
	unsigned short pool_ids[NVDIMM_MAX_TOPO_SIZE];
};



/*
 * Structure for IOCTL data
 */
struct nvdimm_req {
	union {
		/*
		 * Data size for IOCTLs that use *data to represent an array
		 * of elements
		 * example: IOCTL_GET_TOPOLOGY
		 */
		unsigned int data_size;
		/* UUIDs for IOCTLs that require a specific volume
		 * example: IOCTL_GET_VOLUME_DETAILS
		 */
		unsigned char volume_uuid[NVDIMM_UUID_LEN];
		/* Pool ID for IOCTLs that require a specific pool
		 * example: IOCTL_GET_POOL_DETAILS
		 */
		unsigned short pool_id;
		/* DIMM ID for IOCTLs that require a specific nvdimm
		 * example: IOCTL_PASSTHROUGH_CMD
		 */
		unsigned short dimm_id;
	} nvdr_arg1;

	union {
		/* buffer for any structure that is vendor specific or
		 * for IOCTLs that transfer a variable array of structures
		 */
		void *data;
		struct nvdimm_pool_details pool_details;
		struct nvdimm_volume_discovery volume_details;
	} nvdr_arg2;
};

#define nvdr_data_size		nvdr_arg1.data_size
#define nvdr_uuid		nvdr_arg1.uuid
#define nvdr_pool_id		nvdr_arg1.pool_id
#define nvdr_dimm_id		nvdr_arg1.dimm_id

#define nvdr_data		nvdr_arg2.data
#define nvdr_pool_details	nvdr_arg2.pool_details
#define nvdr_volume_details	nvdr_arg2.volume_details
#define nvdr_platform_capabilities	nvdr_arg2.platform_capabilities

#ifdef __linux__
#define NVDIMM_MAGIC (0xDA)

/*Debug IOCTLS*/
#define NVDIMM_LOAD_ACPI_FIT	\
	_IOR(NVDIMM_MAGIC, 0x04, void *)
#define NVDIMM_INIT _IO(NVDIMM_MAGIC, 0x03)


#define NVDIMM_GET_PLATFORM_CAPABILITIES \
	_IOWR(NVDIMM_MAGIC, IOCTL_GET_PLATFORM_CAPABILITIES, struct nvdimm_req)
#define NVDIMM_GET_TOPOLOGY_COUNT	\
	_IO(NVDIMM_MAGIC, IOCTL_GET_TOPOLOGY_COUNT)
#define NVDIMM_GET_DIMM_TOPOLOGY	\
	_IOWR(NVDIMM_MAGIC, IOCTL_GET_TOPOLOGY, struct nvdimm_req)
#define NVDIMM_GET_DIMM_DETAILS		\
	_IOWR(NVDIMM_MAGIC, IOCTL_GET_DIMM_DETAILS, struct nvdimm_req)
#define NVDIMM_GET_POOL_COUNT		\
	_IO(NVDIMM_MAGIC, IOCTL_GET_POOL_COUNT)
#define NVDIMM_GET_POOLS		\
	_IOWR(NVDIMM_MAGIC, IOCTL_GET_POOLS, struct nvdimm_req)
#define NVDIMM_GET_POOL_DETAILS		\
	_IOWR(NVDIMM_MAGIC, IOCTL_GET_POOL_DETAILS, struct nvdimm_req)
#define NVDIMM_GET_VOLUME_COUNT	\
	_IO(NVDIMM_MAGIC, IOCTL_GET_VOLUME_COUNT)
#define NVDIMM_GET_VOLUMES	\
	_IOWR(NVDIMM_MAGIC, IOCTL_GET_VOLUMES, struct nvdimm_req)
#define NVDIMM_GET_VOLUME_DETAILS	\
	_IOWR(NVDIMM_MAGIC, IOCTL_GET_VOLUME_DETAILS, struct nvdimm_req)
#define NVDIMM_CREATE_VOLUME		\
	_IOWR(NVDIMM_MAGIC, IOCTL_CREATE_VOLUME, struct nvdimm_req)
#define NVDIMM_DELETE_VOLUME		\
	_IOWR(NVDIMM_MAGIC, IOCTL_DELETE_VOLUME, struct nvdimm_req)
#define NVDIMM_MODIFY_VOLUME		\
	_IOWR(NVDIMM_MAGIC, IOCTL_MODIFY_VOLUME, struct nvdimm_req)
#define NVDIMM_PASSTHROUGH_CMD	\
	_IOWR(NVDIMM_MAGIC, IOCTL_PASSTHROUGH_CMD, struct nvdimm_req)
#endif

#endif /* NVDIMM_IOCTL_H_ */
