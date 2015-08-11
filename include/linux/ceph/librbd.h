#ifndef _LIBRBD_H
#define _LIBRBD_H

#include <linux/blk-mq.h>
#include <linux/device.h>

/*
 * The basic unit of block I/O is a sector.  It is interpreted in a
 * number of contexts in Linux (blk, bio, genhd), but the default is
 * universally 512 bytes.  These symbols are just slightly more
 * meaningful than the bare numbers they represent.
 */
#define	SECTOR_SHIFT	9
#define	SECTOR_SIZE	(1ULL << SECTOR_SHIFT)

#define RBD_DRV_NAME "rbd"

/*
 * An RBD device name will be "rbd#", where the "rbd" comes from
 * RBD_DRV_NAME above, and # is a unique integer identifier.
 * MAX_INT_FORMAT_WIDTH is used in ensuring DEV_NAME_LEN is big
 * enough to hold all possible device names.
 */
#define DEV_NAME_LEN		32
#define MAX_INT_FORMAT_WIDTH	((5 * sizeof (int)) / 2 + 1)

/*
 * block device image metadata (in-memory version)
 */
struct rbd_image_header {
	/* These six fields never change for a given rbd image */
	char *object_prefix;
	__u8 obj_order;
	__u8 crypt_type;
	__u8 comp_type;
	u64 stripe_unit;
	u64 stripe_count;
	u64 features;		/* Might be changeable someday? */

	/* The remaining fields need to be updated occasionally */
	u64 image_size;
	struct ceph_snap_context *snapc;
	char *snap_names;	/* format 1 only */
	u64 *snap_sizes;	/* format 1 only */
};

enum obj_request_type {
	OBJ_REQUEST_NODATA, OBJ_REQUEST_BIO, OBJ_REQUEST_PAGES, OBJ_REQUEST_SG,
};

enum obj_operation_type {
	OBJ_OP_WRITE,
	OBJ_OP_READ,
	OBJ_OP_DISCARD,
	OBJ_OP_CMP_AND_WRITE,
	OBJ_OP_WRITESAME,
};

struct rbd_img_request;
typedef void (*rbd_img_callback_t)(struct rbd_img_request *);

struct rbd_obj_request;

struct rbd_img_request {
	struct rbd_device	*rbd_dev;
	u64			offset;	/* starting image byte offset */
	u64			length;	/* byte count from offset */
	unsigned long		flags;

	u64			snap_id;	/* for reads */
	struct ceph_snap_context *snapc;	/* for writes */

	struct request		*rq;		/* block request */
	struct rbd_obj_request	*obj_request;	/* obj req initiator */
	void			*lio_cmd_data;	/* lio specific data */

	struct page		**copyup_pages;
	u32			copyup_page_count;
	spinlock_t		completion_lock;/* protects next_completion */
	u32			next_completion;
	rbd_img_callback_t	callback;
        /*
	 * xferred is the bytes that have successfully been transferred.
	 * completed is the bytes that have been accounted for and includes
	 * failures.
	 */
	u64			xferred;/* aggregate bytes transferred */
	u64			completed;/* aggregate bytes completed */
	int			result;	/* first nonzero obj_request result */

	u32			obj_request_count;
	struct list_head	obj_requests;	/* rbd_obj_request structs */

	struct kref		kref;
};

struct rbd_mapping {
	u64                     size;
	u64                     features;
	bool			read_only;
};

struct rbd_client;
struct rbd_spec;
struct rbd_options;

/*
 * a single device
 */
struct rbd_device {
	int			dev_id;		/* blkdev unique id */

	int			major;		/* blkdev assigned major */
	int			minor;
	struct gendisk		*disk;		/* blkdev's gendisk and rq */

	u32			image_format;	/* Either 1 or 2 */
	struct rbd_client	*rbd_client;

	char			name[DEV_NAME_LEN]; /* blkdev name, e.g. rbd3 */

	struct list_head	rq_queue;	/* incoming rq queue */
	spinlock_t		lock;		/* queue, flags, open_count */
	struct work_struct	rq_work;

	struct rbd_image_header	header;
	unsigned long		flags;		/* possibly lock protected */
	struct rbd_spec		*spec;

	char			*header_name;

	struct ceph_file_layout	layout;

	struct ceph_osd_event   *watch_event;
	struct rbd_obj_request	*watch_request;

	struct rbd_spec		*parent_spec;
	u64			parent_overlap;
	atomic_t		parent_ref;
	struct rbd_device	*parent;

	/* protects updating the header */
	struct rw_semaphore     header_rwsem;

	struct rbd_mapping	mapping;

	struct list_head	node;

	/* sysfs related */
	struct device		dev;
	unsigned long		open_count;	/* protected by lock */
};

extern struct rbd_img_request *rbd_img_request_create(
					struct rbd_device *rbd_dev,
					u64 offset, u64 length,
					enum obj_operation_type op_type,
					struct ceph_snap_context *snapc);
extern int rbd_img_cmp_and_write_request_fill(
					struct rbd_img_request *img_request,
					struct scatterlist *cmp_sgl,
					u64 cmp_length,
					struct scatterlist *write_sgl,
					u64 write_length,
					struct page **response_pages,
					u64 response_length);
extern int rbd_img_request_fill(struct rbd_img_request *img_request,
				enum obj_request_type type, void *data_desc);
extern int rbd_img_request_submit(struct rbd_img_request *img_request);
extern void rbd_img_request_put(struct rbd_img_request *img_request);

#endif
