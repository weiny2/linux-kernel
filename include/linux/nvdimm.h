#ifndef NVDIMM_H_
#define NVDIMM_H_

#define MAX_REGIONS_PER_VOLUME 60

#define APPNAME         "NVDIMM"
#define LEVEL           APPNAME

/*! Maximum number of DIMMs possible for a given memory topology*/
#define	NVDIMM_MAX_TOPO_SIZE	96
/*! Maximum number of DIMMs possible for an memory NUMA interleave*/
#define MAX_INTERLEAVE 12
#define	NVDIMM_UUID_LEN	16 /*!< UUID length*/
/*! Volume 'Friendly Name' buffer length*/
#define	NVDIMM_VOL_FRIENDLY_NAME_LEN	32
#define	NVDIMM_VOL_PURPOSE_LEN	100 /*!< Volume 'Purpose' buffer length*/

#define CTRL_NAME "pmem"

enum nvdimm_format {
	NO_FORMAT = 0,
	AEP_DIMM = 1,
};

enum nvdimm_health {
	NVDIMM_UNINITIALIZED = 0,
	NVDIMM_INITIALIZED = 1,
};

enum pool_attributes {
	/*!Pool memory supports persistent memory access*/
	POOL_PM_CAPABLE = (1 << 2),
	POOL_DPA = (1 << 6), /*!< Pool memory expects DPAs for block I/O*/
	POOL_SPA = (1 << 7), /*!< Pool memory expects SPAs for block I/O*/
	/*TODO: Cached needs more clarification*/
	POOL_CACHED = (1 << 16), /*!< Pool is being used for file caching*/
	POOL_MIRRORED = (1 << 17), /*!< Pool memory is being mirrored by HW*/
	POOL_ENCRYPTED = (1 << 18), /*!< Pool memory is encrypted by HW*/
};

enum volume_attributes {
	VOLUME_ENABLED = (1 << 0), /*!< Volume has been enabled by user*/
	 /*!Volume is not missing any extents/labels*/
	VOLUME_COMPLETE = (1 << 1),
	/*! Volume supports persistent memory access*/
	VOLUME_PM_CAPABLE = (1 << 2),
	VOLUME_DPA = (1 << 6), /*!< LBAs are converted to DPAs*/
	VOLUME_SPA = (1 << 7), /*!< LBAs are converted to SPAs*/
	/*! Volume enforces single sector power fail atomicity*/
	VOLUME_ATOMIC = (1 << 8),
	/*! Atomicity is supported through the use of a BTT and freelist*/
	VOLUME_BTT_ATOMIC = (1 << 9),
};

#endif /* NVDIMM_H_ */
