#ifndef TARGET_CORE_TCM_RBD_H
#define TARGET_CORE_TCM_RBD_H

#define TCM_RBD_VERSION		"4.0"

#define TCM_RBD_HAS_UDEV_PATH		0x01

struct scatterlist;

struct tcm_rbd_dev {
	struct se_device dev;
	struct rbd_device *rbd_dev;

	struct scatterlist *cmp_and_write_sg;
	struct page **cmp_and_write_pages;

	unsigned char bd_udev_path[SE_UDEV_PATH_LEN];
	u32 bd_flags;
	struct block_device *bd;
	bool bd_readonly;
} ____cacheline_aligned;

#endif /* TARGET_CORE_IBLOCK_H */
