#ifndef NVDIMM_CORE_H_
#define NVDIMM_CORE_H_

#include <linux/nvdimm.h>
#include <os_adapter.h>
#include <linux/nvdimm_acpi.h>

/************************************************
 *  NVDIMM STRUCTS
 ***********************************************/

/*TODO: STRINGs not handled correctly, a function in dmi_scan exists for this*/
/*SMBIOS Type17 Memory Device Structure*/
/*SMBIOS Reference Specification Version 2.8.0*/
struct memdev_dmi_entry {
	__u8 type;
	__u8 length;
	__u16 handle;
	__u16 phys_mem_array_handle;
	__u16 mem_err_info_handle;
	__u16 total_width;
	__u16 data_width;
	__u16 size;
	__u8 form;
	__u8 device_set;
	__u8 device_locator;
	__u8 bank_locator;
	__u8 memory_type;
	__u16 type_detail;
	__u16 speed;
	__u8 manufacturer;
	__u8 serial_number;
	__u8 asset_tag;
	__u8 part_number;
	__u8 attributes;
	__u32 extended_size;
	__u16 conf_mem_clk_speed;
	__u16 min_voltage;
	__u16 max_voltage;
	__u16 configured_voltage;
} __packed;

/*SMBIOS Type20 Memory Device Mapped Address*/
/*SMBIOS Reference Specification Version 2.8.0*/
struct memdev_mapped_addr_entry {
	__u8 type;
	__u8 length;
	__u16 handle;
	__u32 start_address;
	__u32 end_address;
	__u16 device_handle;
	__u16 memory_array_handle;
	__u8 partition_row_position;
	__u8 interleave_position;
	__u8 interleaved_data_depth;
	__u64 extended_start_addr;
	__u64 extended_end_addr;
} __packed;

struct nvdimm_ids {
	__u16 vendor_id;
	__u16 device_id;
	__u16 rid;
	__u16 fmt_interface_code;
};

/*Generic*/
struct nvdimm {
	struct list_head dimm_node;
	__u16 physical_id; /*SMBIOS Type 17 handle corresponding
	 to this memory device*/
	__u16 imc_id; /*refers to a memory controller that this DIMM
	 belongs to for the given socket. The ID here is a
	 logical ID corresponding to a TBD table*/
	__u16 socket_id; /*refers to the socket to which this DIMM
	 is attached via memory controller. Socket ID
	 is a logic id that corresponds to a TBD table*/
	__u8 health;
	__u8 rsvd;

	struct nvdimm_ids ids;
	const struct nvdimm_ops *dimm_ops;
	const struct nvdimm_ioctl_ops *ioctl_ops;
	struct memdev_dmi_entry *dmi_physical_dev;
	struct memdev_mapped_addr_entry *dmi_device_mapped_addr;
	void *private; /*Hide Unique DIMM type information here,
	 like CR specific data*/
};

/*Generic*/
/*Written as it appears on disk*/
struct volume_region {
	__le64 vol_start; /*Starting LBA in the vol for this region*/
	__le64 vol_blocks; /*Number of LBA blocks in this region*/
	__le64 dpa_start; /*Starting DPA for this region*/
	__le64 dpa_length; /*length of the region in DPA space, Bytes*/
	__le64 interleave_set; /*Number of PM devices in one interleave set */
	__le64 interleave_size; /*Length of one interleave segment*/
	__le64 interleave_offset; /*Starting offset of this region on the
	 interleave window*/
	__le64 pad;/*reserved must be zero*/
};

/*Generic*/
/*Written as it appears on disk*/
struct volume_label {
	__u32 magic_num; /*magic number identifying as a CR vol label*/
	__le16 major; /*Major revision number for the CR vol format*/
	__le16 minor; /*Minor revision number for the CR vol format*/
	__le64 generation; /*generation number of this label*/
	__u8 uuid[16];
	__u8 name[64]; /*ASCII string denoting the user-assigned name of
	 the CR Volume. */
	__u8 purpose[64]; /*User defined volume purpose*/
	__le64 creation_time; /*Time in seconds since 1970 epoch*/
	__le64 block_size;/*Logical Block size in Bytes of the vol*/
	__le64 block_count;/*Number of Logical Blocks in volume*/
	__le32 vol_regions;/*Number of PM regions making up this volume*/
	__le32 dev_regions;/*Number of PM regions on PM device that
	 contain this label*/
	__le16 physical_id;/*SMBIOS Type 17 handle corresponding
	 to the nvdimm this is being written to*/
	__u8 rsvd[2]; /* 0 */
	__le32 volume_attributes;
	struct volume_region regions[MAX_REGIONS_PER_VOLUME];
	__u8 pad[48]; /* pad necessary to make struct size 4096, should be 0 */
	__le64 checksum; /*Checksum for the previous 4088 bytes of this label*/
} __packed;

/*Generic - Making a guess that all NVDIMMs must use a volume label space*/
struct label_info {
	struct list_head label_node;
	struct volume_label *label;
	__u64 primary_label_offset; /*Offset from start of volume label space
	 to the first copy of the volume label*/
	__u64 secondary_label_offset; /*Offset from the start of the volume
	 label space to the second copy of the
	volume label*/
};

/************************************************
 *  NVM POOL STRUCTS
 ***********************************************/

/*CR Specific?*/
struct extent_set {
	struct list_head extent_set_node;
	phys_addr_t spa_start;
	size_t set_size;
	__u32 pool_id;
	__u8 pm_capable; /*Identifies if the extent set is PM capable*/
	__u8 numa_node;
	__u8 rsvd[2];
	__u32 interleave_size;
	__u8 rsvd2[4];
	__u16 physical_id[MAX_INTERLEAVE]; /*Physical IDs of the DIMMs that
	 comprise this extent set*/
};

/*CR Specific?*/
struct pool {
	struct list_head pool_list;
	__u16 pool_id; /*Pool Id 0 -> 63999 represent PM capable pools*/
	/*Pool id 64000+ represent non-pm capable pools*/
	__u16 numa_node; /*Identifies what NUMA if any a pool belongs too*/
	/*TODO: more information about numa @include/linux/topology.h*/
	__u32 interleave_size; /*The interleave size if any of the pool*/
	__u8 interleave_way;
	__u8 rsvd[3];
	__u32 pool_attributes;
	size_t total_capacity; /*Current total capacity of the pool*/
	struct list_head extent_set_list;
};

/************************************************
 *  NVM VOLUME STRUCTS
 ***********************************************/
/*Generic*/
struct nvm_volume {
	struct list_head volume_node;
	__u8 volume_uuid[16]; /*universal id*/
	__u16 volume_id; /*local id*/
	__u8 rsvd[2];
	__u32 interleave_size;
	char name[64]; /*User defined friendly name*/
	char purpose[64]; /*User defined volume purpose*/
	time_t creation_time;
	__u32 volume_attributes;
	__u32 label_count;
	struct list_head label_list;
	__u64 block_size;
	__u64 block_count;

	struct gendisk *gd;
	struct request_queue *queue;

	struct mutex extent_set_lock;
	struct list_head extent_set_list;
};

#ifdef __KERNEL__
#include <linux/miscdevice.h>
#endif

struct pmem_dev {
	spinlock_t volume_lock;
	spinlock_t nvdimm_lock;
	spinlock_t nvm_pool_lock;

	struct list_head volumes;
	struct list_head nvdimms;
	struct list_head nvm_pools;

	struct fit_header *fit_head;
#ifdef __KERNEL__
	struct miscdevice miscdev;
	struct kref kref;
#endif
};

struct nvdimm_ops {
	int (*initialize_dimm)(struct nvdimm *, struct fit_header *);
	int (*remove_dimm)(struct nvdimm *);
	int (*write_label)(struct nvdimm *, struct label_info *);
	int (*read_labels)(struct nvdimm *, struct list_head *);
	int (*read)(struct nvdimm *, unsigned long, unsigned long, char*);
	int (*write)(struct nvdimm *, unsigned long, unsigned long, char*);
	/*int (*clear_error) (struct nvdimm *, void *, size_t, void *)*/
};

struct nvm_pool_ops {
	int (*initialize_pools)(struct pmem_dev *);
	int (*destroy_pools)(struct pmem_dev *);
	struct extent_set (*provision_extent)(struct pool *, size_t);
	int (*return_extent)(struct pool *, struct extent_set *);
	__u8 rsvd[4];
};

struct nvdimm_ioctl_ops {
	int (*get_dimm_details)(struct pmem_dev *, __u32);
	int (*passthrough_cmd)(struct nvdimm *, void __user *);
};

struct nvdimm_driver {
	struct module *owner;
	struct list_head nvdimm_driver_node;
	const struct nvdimm_ids *ids;

	int (*probe) (struct pmem_dev *);
	void (*remove) (struct pmem_dev *);

	const struct nvdimm_ops *dimm_ops;
	const struct nvm_pool_ops *pool_ops;
	const struct nvdimm_ioctl_ops *ioctl_ops;
};

#ifdef __KERNEL__
void __iomem *cr_get_virt(phys_addr_t spa_addr,
		struct memdev_spa_rng_tbl *ctrl_tbl);
int get_dmi_memdev(struct nvdimm *dimm);
#endif
int gen_nvdimm_init(struct pmem_dev *dev);
void gen_nvdimm_exit(struct pmem_dev *dev);

#ifndef __linux__
int get_dmi_memdev(struct nvdimm *dimm);
#endif
struct nvdimm_driver *find_nvdimm_driver(const struct nvdimm_ids *ids);
int match_nvdimm_ids(const struct nvdimm_ids *first_ids,
		const struct nvdimm_ids *second_ids);
#endif
