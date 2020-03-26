/* SPDX-License-Identifier: GPL-2.0-only */
/*
 * xlink Linux Kernel Platform API
 *
 * Copyright (C) 2018-2019 Intel Corporation
 *
 */
#include <linux/init.h>
#include <linux/module.h>
#include <linux/device.h>
#include <linux/kernel.h>
#include <linux/platform_device.h>
#include <linux/slab.h>
#include <linux/dma-mapping.h>
#include <linux/of.h>
#include <linux/of_address.h>
#include <linux/of_platform.h>
#include <linux/of_reserved_mem.h>
#include <linux/keembay-vpu-ipc.h>
#include <linux/xlink-ipc.h>

/* used to get xlink device information */
#define XLINK_IPC_MAX_NMB_DEVICES 4
#define XLINK_IPC_MAX_DEVICE_NAME_SIZE 12

/* used to extract fields from and create xlink sw device id */
#define SW_DEVICE_ID_INTERFACE_SHIFT 24U
#define SW_DEVICE_ID_INTERFACE_MASK 0x7
#define SW_DEVICE_ID_INTERFACE_SMASK \
		(SW_DEVICE_ID_INTERFACE_MASK << SW_DEVICE_ID_INTERFACE_SHIFT)
#define SW_DEVICE_ID_INTERFACE_IPC_VALUE 0x3
#define SW_DEVICE_ID_INTERFACE_IPC_SVALUE \
		(SW_DEVICE_ID_INTERFACE_IPC_VALUE << SW_DEVICE_ID_INTERFACE_SHIFT)
#define SW_DEVICE_ID_VPU_ID_SHIFT 1U
#define SW_DEVICE_ID_VPU_ID_MASK 0x7
#define SW_DEVICE_ID_VPU_ID_SMASK \
		(SW_DEVICE_ID_VPU_ID_MASK << SW_DEVICE_ID_VPU_ID_SHIFT)
#define GET_VPU_ID_FROM_SW_DEVICE_ID(id) \
		((id >> SW_DEVICE_ID_VPU_ID_SHIFT) & SW_DEVICE_ID_VPU_ID_MASK)
#define GET_SW_DEVICE_ID_FROM_VPU_ID(id) \
		(((id << SW_DEVICE_ID_VPU_ID_SHIFT) & SW_DEVICE_ID_VPU_ID_SMASK) \
		| SW_DEVICE_ID_INTERFACE_IPC_SVALUE)

/* the maximum buffer size for volatile xlink operations */
#define XLINK_MAX_BUF_SIZE 128U

/* indices used to retrieve reserved memory from the dt */
#define LOCAL_XLINK_IPC_BUFFER_IDX	0
#define REMOTE_XLINK_IPC_BUFFER_IDX	1

/* index used to retrieve the vpu ipc device phandle from the dt */
#define VPU_IPC_DEVICE_PHANDLE_IDX	1

/* vpu firmware image file name */
#define VPU_FIRMWARE_NAME "vpu.bin"

/* the timeout (in ms) used to wait for the vpu ready message */
/* TODO: reduce timeout to 3000, waiting longer for PSS platform */
#define XLINK_VPU_WAIT_FOR_READY_MS (3000000)

/* xlink buffer memory region */
struct xlink_buf_mem {
	struct device *dev;		/* child device managing the memory region */
	void *vaddr;			/* the virtual address of the memory region */
	dma_addr_t dma_handle; 	/* the physical address of the memory region */
	size_t size;			/* the size of the memory region */
};

/* xlink buffer pool */
struct xlink_buf_pool {
	void *buf;			/* pointer to the start of pool area */
	size_t buf_cnt;		/* pool size (i.e., number of buffers) */
	size_t idx;			/* current index */
	spinlock_t lock;	/* the lock protecting this pool */
};

/* xlink ipc device */
struct xlink_ipc_dev {
	struct platform_device *pdev;			/* pointer to platform device */
	u32 vpu_id;								/* the vpu slice id */
	u32 sw_device_id;						/* the sw device id */
	const char *device_name;				/* the vpu device name */
	struct xlink_buf_mem local_xlink_mem;	/* tx buffer memory region */
	struct xlink_buf_mem remote_xlink_mem;	/* rx buffer memory region */
	struct xlink_buf_pool xlink_buf_pool;	/* tx buffer pool */
	struct device *vpu_dev;					/* pointer to vpu ipc device */
};

/* used for single-slice backwards compatibility */
/* TODO: remove when kmb dt is updated for backwards compatibility */
#define VPU_SLICE_0_ID 0
#define VPU_SLICE_0_DEVICE_NAME "vpu-slice-0"
#define VPU_SLICE_0_SW_DEVICE_ID 0
static struct xlink_ipc_dev *xlink0_dev;

/* helper functions to get the xlink_ipc_dev data structure */
static struct xlink_ipc_dev *get_xlink_dev_by_sw_device_id(u32 sw_device_id);
static struct xlink_ipc_dev *get_xlink_dev_by_name(const char *device_name);

/*
 * Functions related to reserved-memory sub-devices.
 */

/*
 * xlink_reserved_memory_remove() - Removes the reserved memory sub-devices.
 *
 * @xlink_dev: [in] The xlink ipc device with reserved memory sub-devices.
 */
static void xlink_reserved_memory_remove(struct xlink_ipc_dev *xlink_dev)
{
	device_unregister(xlink_dev->local_xlink_mem.dev);
	device_unregister(xlink_dev->remote_xlink_mem.dev);
}

/*
 * xlink_reserved_mem_release() - Reserved memory release callback function.
 *
 * @dev: [in] The reserved memory sub-device.
 */
static void xlink_reserved_mem_release(struct device *dev)
{
	of_reserved_mem_device_release(dev);
}

/*
 * get_xlink_reserved_mem_size() - Gets the size of the reserved memory region.
 *
 * @dev: [in] The device the reserved memory region is allocated to.
 * @idx: [in] The reserved memory region's index in the phandle table.
 *
 * Return: The reserved memory size, 0 on failure.
 */
static resource_size_t get_xlink_reserved_mem_size(struct device *dev, int idx)
{
	struct resource mem;
	struct device_node *np;
	int rc;

	np = of_parse_phandle(dev->of_node, "memory-region", idx);
	if (!np) {
		dev_err(dev, "Couldn't find memory-region %d\n", idx);
		return 0;
	}

	rc = of_address_to_resource(np, 0, &mem);
	if (rc) {
		dev_err(dev, "Couldn't map address to resource %d\n", idx);
		return 0;
	}
	return resource_size(&mem);
}

/*
 * init_xlink_reserved_mem_dev() - Initializes the reserved memory sub-devices.
 *
 * @dev:	[in] The parent device of the reserved memory sub-device.
 * @name:	[in] The name to assign to the memory region.
 * @idx: 	[in] The reserved memory region's index in the phandle table.
 *
 * Return: The initialized sub-device, NULL on failure.
 */
static struct device *init_xlink_reserved_mem_dev(struct device *dev,
		const char *name, int idx)
{
	struct device *child;
	int rc;

	child = devm_kzalloc(dev, sizeof(*child), GFP_KERNEL);
	if (!child)
		return NULL;

	device_initialize(child);
	dev_set_name(child, "%s:%s", dev_name(dev), name);
	dev_err(dev, " dev_name %s, name %s\n", dev_name(dev), name);
	child->parent = dev;
	child->dma_mask = dev->dma_mask;
	child->coherent_dma_mask = dev->coherent_dma_mask;
	/* set up dma configuration using information from parent's dt node */
	rc = of_dma_configure(child, dev->of_node, true);
	child->release = xlink_reserved_mem_release;

	rc = device_add(child);
	if (rc)
		goto err;
	rc = of_reserved_mem_device_init_by_idx(child, dev->of_node, idx);
	if (rc) {
		dev_err(dev, "Couldn't get reserved memory with idx = %d, %d\n",
				idx, rc);
		device_del(child);
		goto err;
	}
	return child;

err:
	put_device(child);
	return NULL;
}

/*
 * xlink_reserved_memory_init() - Initialize reserved memory for the device.
 *
 * @xlink_dev:	[in] The xlink ipc device the reserved memory is allocated to.
 *
 * Return: 0 on success, negative error code otherwise.
 */
static int xlink_reserved_memory_init(struct xlink_ipc_dev *xlink_dev)
{
	struct device *dev = &xlink_dev->pdev->dev;

	xlink_dev->local_xlink_mem.dev = init_xlink_reserved_mem_dev(
			dev, "xlink_local_reserved", LOCAL_XLINK_IPC_BUFFER_IDX);
	if (!xlink_dev->local_xlink_mem.dev)
		return -ENOMEM;

	xlink_dev->local_xlink_mem.size =
			get_xlink_reserved_mem_size(dev, LOCAL_XLINK_IPC_BUFFER_IDX);

	xlink_dev->remote_xlink_mem.dev = init_xlink_reserved_mem_dev(
			dev, "xlink_remote_reserved", REMOTE_XLINK_IPC_BUFFER_IDX);
	if (!xlink_dev->remote_xlink_mem.dev) {
		device_unregister(xlink_dev->local_xlink_mem.dev);
		return -ENOMEM;
	}

	xlink_dev->remote_xlink_mem.size =
			get_xlink_reserved_mem_size(dev, REMOTE_XLINK_IPC_BUFFER_IDX);
	return 0;
}

/*
 * xlink_reserved_memory_alloc() - Allocate reserved memory for the device.
 *
 * @xlink_dev:	[in] The xlink ipc device.
 *
 * Return: 0 on success, negative error code otherwise.
 */
static int xlink_reserved_memory_alloc(struct xlink_ipc_dev *xlink_dev)
{
	xlink_dev->local_xlink_mem.vaddr =
			dmam_alloc_coherent(xlink_dev->local_xlink_mem.dev,
			xlink_dev->local_xlink_mem.size,
			&xlink_dev->local_xlink_mem.dma_handle,
			GFP_KERNEL);
	if (!xlink_dev->local_xlink_mem.vaddr) {
		dev_err(&xlink_dev->pdev->dev,
				"Failed to allocate from local reserved memory.\n");
		return -ENOMEM;
	}
	xlink_dev->remote_xlink_mem.vaddr = dmam_alloc_coherent(
			xlink_dev->remote_xlink_mem.dev,
			xlink_dev->remote_xlink_mem.size,
			&xlink_dev->remote_xlink_mem.dma_handle,
			GFP_KERNEL);
	if (!xlink_dev->remote_xlink_mem.vaddr) {
		dev_err(&xlink_dev->pdev->dev,
				"Failed to allocate from remote reserved memory.\n");
		return -ENOMEM;
	}
	dev_info(&xlink_dev->pdev->dev,
			"Local vaddr 0x%px paddr 0x%pad size 0x%zX\n",
			xlink_dev->local_xlink_mem.vaddr,
			&xlink_dev->local_xlink_mem.dma_handle,
			xlink_dev->local_xlink_mem.size);
	dev_info(&xlink_dev->pdev->dev,
			"Remote vaddr 0x%px paddr 0x%pad size 0x%zX\n",
			xlink_dev->remote_xlink_mem.vaddr,
			&xlink_dev->remote_xlink_mem.dma_handle,
			xlink_dev->remote_xlink_mem.size);
	return 0;
}

/*
 * init_xlink_buf_pool() - Initialize the device's tx buffer pool.
 *
 * @xlink_dev:	[in] The xlink ipc device.
 *
 * Return: 0 on success.
 */
static int init_xlink_buf_pool(struct xlink_ipc_dev *xlink_dev)
{
	struct xlink_buf_mem *mem = &xlink_dev->local_xlink_mem;
	memset(mem->vaddr, 0, mem->size);
	xlink_dev->xlink_buf_pool.buf = mem->vaddr;
	xlink_dev->xlink_buf_pool.buf_cnt =
			mem->size / XLINK_MAX_BUF_SIZE;
	xlink_dev->xlink_buf_pool.idx = 0;
	dev_info(&xlink_dev->pdev->dev, "xlink Buffer Pool size: %zX\n",
			xlink_dev->xlink_buf_pool.buf_cnt);
	spin_lock_init(&xlink_dev->xlink_buf_pool.lock);
	return 0;
}

/*
 * xlink_phys_to_virt() - Convert an xlink physical addresses to a virtual one.
 *
 * @xlink_mem:	[in] The memory region where the physical address is located.
 * @paddr:		[in] The physical address to be converted to a virtual one.
 *
 * Return: 	The corresponding virtual address, or NULL if the physical address
 *	   		is not in the expected memory range.
 */
static void *xlink_phys_to_virt(const struct xlink_buf_mem *xlink_mem,
		u32 paddr)
{
	if (unlikely(paddr < xlink_mem->dma_handle) ||
			 paddr >= (xlink_mem->dma_handle + xlink_mem->size))
		return NULL;
	return xlink_mem->vaddr + (paddr - xlink_mem->dma_handle);
}

/*
 * xlink_virt_to_phys() - Convert an xlink virtual addresses to a physical one.
 *
 * @xlink_mem:	[in]  The memory region where the physical address is located.
 * @vaddr:   	[in]  The virtual address to be converted to a physical one.
 * @paddr:   	[out] Where to store the computed physical address.
 *
 * Return: 0 on success, negative error code otherwise.
 */
static int xlink_virt_to_phys(struct xlink_buf_mem *xlink_mem, void *vaddr,
		u32 *paddr)
{
	if (unlikely((xlink_mem->dma_handle + xlink_mem->size) > 0xFFFFFFFF))
		return -EINVAL;
	if (unlikely(vaddr < xlink_mem->vaddr ||
			vaddr >= (xlink_mem->vaddr + xlink_mem->size)))
		return -EINVAL;
	*paddr = xlink_mem->dma_handle + (vaddr - xlink_mem->vaddr);
	return 0;
}

/*
 * get_next_xlink_buf() - Get next xlink buffer from an xlink device's pool.
 *
 * @xlink_dev:	[in]  The xlink ipc device to get a buffer from.
 * @buf:		[out] Where to store the reference to the next buffer.
 * @size:		[in]  The size of the buffer to get.
 *
 * Return: 0 on success, negative error code otherwise.
 */
static int get_next_xlink_buf(struct xlink_ipc_dev *xlink_dev, void **buf,
		int size)
{
	struct xlink_buf_pool *pool;
	unsigned long flags;

	if (!xlink_dev)
		return -ENODEV;

	if (size > XLINK_MAX_BUF_SIZE)
		return -EINVAL;

	pool = &xlink_dev->xlink_buf_pool;

	spin_lock_irqsave(&pool->lock, flags);
	if (pool->idx == pool->buf_cnt) {
		/* reached end of buffers - wrap around */
		pool->idx = 0;
	}
	*buf = pool->buf + (pool->idx * XLINK_MAX_BUF_SIZE);
	pool->idx++;
	spin_unlock_irqrestore(&pool->lock, flags);
	return 0;
}

/*
 * Functions related to the vpu ipc device reference.
 */

/*
 * vpu_ipc_device_put() - Release the vpu ipc device held by the xlink device.
 *
 * @xlink_dev:	[in] The xlink ipc device.
 */
static void vpu_ipc_device_put(struct xlink_ipc_dev *xlink_dev)
{
	put_device(xlink_dev->vpu_dev);
}

/*
 * vpu_ipc_device_put() - Get the vpu ipc device reference for the xlink device.
 *
 * @xlink_dev:	[in] The xlink ipc device.
 *
 * Return: 0 on success, negative error code otherwise.
 */
static int vpu_ipc_device_get(struct xlink_ipc_dev *xlink_dev)
{
	struct device *dev = &xlink_dev->pdev->dev;
	struct device_node *np;
	struct platform_device *pdev;

	np = of_parse_phandle(dev->of_node, "intel,keembay-vpu-ipc", 0);
	if (!np) {
		dev_warn(dev, "Cannot find phandle to VPU IPC device\n");
		/*
		 * For (temporary) retro-compatibility, if the VPU IPC phandle is
		 * not defined, we try to find a VPU IPC device whose dt node is a
		 * sibling of the xlink IPC node.
		 */
		dev_warn(dev, "Fallback: looking for a sibling VPU IPC node\n");
		np = of_get_compatible_child(dev->parent->of_node,
				"intel,keembay-vpu-ipc");
		if (!np) {
			dev_err(dev, "Couldn't find a sibling VPU IPC node\n");
			return -ENODEV;
		}
	}

	pdev = of_find_device_by_node(np);
	if (!pdev) {
		dev_info(dev, "IPC device not probed\n");
		of_node_put(np);
		return -EPROBE_DEFER;
	}

	xlink_dev->vpu_dev = get_device(&pdev->dev);
	of_node_put(np);

	dev_info(dev, "Using IPC device: %s\n", dev_name(xlink_dev->vpu_dev));
	return 0;
}

/*
 * xlink platform api - ipc interface functions
 */

/*
 * xlink_ipc_connect() - platform connect interface.
 *
 * @device_name:	[in]  The name of the device to connect to.
 * @fd:				[out] The xlink ipc device connected to.
 *
 * Return: 0 on success, negative error code otherwise.
 */
int xlink_ipc_connect(const char *device_name, void **fd)
{
	struct xlink_ipc_dev *xlink_dev = NULL;

	if (!device_name || !fd)
		return -EINVAL;

	xlink_dev = get_xlink_dev_by_name(device_name);
	if (!xlink_dev)
		return -ENODEV;

	*fd = xlink_dev;
	return 0;
}
EXPORT_SYMBOL(xlink_ipc_connect);

/*
 * xlink_ipc_write() - platform write interface.
 *
 * @fd:			[in]     The xlink ipc operation descriptor.
 * @data:		[in]     The data buffer to write.
 * @size:		[in-out] The amount of data to write/written.
 * @timeout:	[in]     The time (in ms) to wait before timing out.
 *
 * Return: 0 on success, negative error code otherwise.
 */
int xlink_ipc_write(void *fd, void *data, size_t * const size, u32 timeout)
{
	int rc = 0;
	u32 paddr;
	void *vaddr = 0;
	struct xlink_ipc_dev *xlink_dev;
	struct xlink_ipc_fd *ipc = (struct xlink_ipc_fd *)fd;

	if (!ipc)
		return -EINVAL;

	xlink_dev = get_xlink_dev_by_sw_device_id(ipc->sw_device_id);
	if (!xlink_dev)
		return -ENODEV;

	if (ipc->is_volatile) {
		if (get_next_xlink_buf(xlink_dev, &vaddr, XLINK_MAX_BUF_SIZE)) {
			return -ENOMEM;
		} else {
			memcpy(vaddr, data, *size);
		}
		if (xlink_virt_to_phys(&xlink_dev->local_xlink_mem, vaddr, &paddr)) {
			return -ENXIO;
		}
	} else {
		paddr = *(u32 *)data;
	}
	rc = intel_keembay_vpu_ipc_send(xlink_dev->vpu_dev, ipc->node, ipc->chan,
			paddr, *size);
	return rc;
}
EXPORT_SYMBOL(xlink_ipc_write);

/*
 * xlink_ipc_read() - platform read interface.
 *
 * @fd:			[in]     The xlink ipc operation descriptor.
 * @data:		[out]    The data buffer to read into.
 * @size:		[in-out] The amount of data to read/was read.
 * @timeout:	[in]     The time (in ms) to wait before timing out.
 *
 * Return: 0 on success, negative error code otherwise.
 */
int xlink_ipc_read(void *fd, void *data, size_t * const size, u32 timeout)
{
	int rc = 0;
	u32 addr = 0;
	void *vaddr = 0;
	struct xlink_ipc_dev *xlink_dev;
	struct xlink_ipc_fd *ipc = (struct xlink_ipc_fd *)fd;

	if (!ipc)
		return -EINVAL;

	xlink_dev = get_xlink_dev_by_sw_device_id(ipc->sw_device_id);
	if (!xlink_dev)
		return -ENODEV;

	rc = intel_keembay_vpu_ipc_recv(xlink_dev->vpu_dev, ipc->node, ipc->chan,
			&addr, size, timeout);

	if (ipc->is_volatile) {
		vaddr = xlink_phys_to_virt(&xlink_dev->remote_xlink_mem, addr);
		if (vaddr) {
			memcpy(data, vaddr, *size);
		} else {
			return -ENXIO;
		}
	} else {
		*(u32 *)data = addr;
	}
	return rc;
}
EXPORT_SYMBOL(xlink_ipc_read);

/*
 * xlink_ipc_get_device_list() - platform get device list interface.
 *
 * @sw_device_id_list:	[out]  The list of devices found.
 * @num_devices:		[out]  The number of devices found.
 * @pid:				[in]   The product id used to filter the device list.
 *
 * Return: 0 on success, negative error code otherwise.
 */
int xlink_ipc_get_device_list(u32 *sw_device_id_list, u32 *num_devices,
		int pid)
{
	int i = 0;
	u32 sw_device_id = 0;
	struct xlink_ipc_dev *xlink_dev;

	if (!sw_device_id_list || !num_devices)
		return -EINVAL;

	for (i = 0; i < XLINK_IPC_MAX_NMB_DEVICES; i++) {
		sw_device_id = GET_SW_DEVICE_ID_FROM_VPU_ID(i);
		xlink_dev = get_xlink_dev_by_sw_device_id(sw_device_id);
		if(!xlink_dev)
			break;

		*sw_device_id_list = xlink_dev->sw_device_id;
		sw_device_id_list++;
	}
	*num_devices = i;
	return 0;
}
EXPORT_SYMBOL(xlink_ipc_get_device_list);

/*
 * xlink_ipc_get_device_name() - platform get device name interface.
 *
 * @sw_device_id:	[in]  The sw device id of the device to get name of.
 * @device_name:	[out] The name of the xlink ipc device.
 * @name_size:		[in]  The maximum size of the name.
 *
 * Return: 0 on success, negative error code otherwise.
 */
int xlink_ipc_get_device_name(u32 sw_device_id, char *device_name,
		size_t name_size)
{
	struct xlink_ipc_dev *xlink_dev;
	size_t size;

	if (!device_name)
		return -EINVAL;

	xlink_dev = get_xlink_dev_by_sw_device_id(sw_device_id);
	if (!xlink_dev)
		return -ENODEV;

	size = (name_size > XLINK_IPC_MAX_DEVICE_NAME_SIZE)
			? XLINK_IPC_MAX_DEVICE_NAME_SIZE
			: name_size;
	strncpy(device_name, xlink_dev->device_name, size);
	return 0;
}
EXPORT_SYMBOL(xlink_ipc_get_device_name);

/*
 * xlink_ipc_get_device_status() - platform get device status interface.
 *
 * @device_name:	[in]  The name of the device to get status from.
 * @device_status:	[out] The device status.
 *
 * Return: 0 on success, negative error code otherwise.
 */
int xlink_ipc_get_device_status(const char *device_name, u32 *device_status)
{
	struct xlink_ipc_dev *xlink_dev = NULL;

	if (!device_name || !device_status)
		return -EINVAL;

	xlink_dev = get_xlink_dev_by_name(device_name);
	if (!xlink_dev)
		return -ENODEV;

	*device_status = intel_keembay_vpu_status(xlink_dev->vpu_dev);
	return 0;
}
EXPORT_SYMBOL(xlink_ipc_get_device_status);

/*
 * xlink_ipc_boot_device() - platform boot device interface.
 *
 * @device_name:	[in] The name of the xlink ipc device to boot.
 * @binary_path:	[in] The path to the firmware image to boot.
 *
 * Return: 0 on success, negative error code otherwise.
 */
int xlink_ipc_boot_device(const char *device_name, const char *binary_path)
{
	int rc = 0;
	enum intel_keembay_vpu_state state;
	struct xlink_ipc_dev *xlink_dev = NULL;

	/* ignore parameter and use hard coded file name */
	binary_path = VPU_FIRMWARE_NAME;

	if (!device_name || !binary_path)
		return -EINVAL;

	xlink_dev = get_xlink_dev_by_name(device_name);
	if (!xlink_dev)
		return -ENODEV;

	pr_info("\nStart VPU %s - %s\n", device_name, binary_path);
	pr_info("Starting device %u: %s (%s)\n", xlink_dev->vpu_id,
			xlink_dev->device_name, dev_name(xlink_dev->vpu_dev));
	rc = intel_keembay_vpu_startup(xlink_dev->vpu_dev, binary_path);
	if (rc) {
		pr_err("Failed to start VPU: %d\n", rc);
		return -EBUSY;
	}
	pr_info("Successfully started VPU!\n");

	/* Wait for VPU to be READY */
	rc = intel_keembay_vpu_wait_for_ready(xlink_dev->vpu_dev,
			XLINK_VPU_WAIT_FOR_READY_MS);
	if (rc) {
		pr_err("Tried to start VPU but never got READY.\n");
		return -EBUSY;
	}
	pr_info("Successfully synchronised state with VPU!\n");

	/* Check state */
	state = intel_keembay_vpu_status(xlink_dev->vpu_dev);
	if (state != KEEMBAY_VPU_READY) {
		pr_err("VPU was not ready, it was %d\n", state);
		return -EBUSY;
	}
	pr_info("VPU was ready.\n");
	return 0;
}
EXPORT_SYMBOL(xlink_ipc_boot_device);

/*
 * xlink_ipc_reset_device() - platform reset device interface.
 *
 * @fd:						[in] The xlink ipc device to reset.
 * @operating_frequency:	[in] The system operating frequency to use.
 *
 * Return: 0 on success, negative error code otherwise.
 */
int xlink_ipc_reset_device(void *fd, u32 operating_frequency)
{
	int rc = 0;
	enum intel_keembay_vpu_state state;
	struct xlink_ipc_dev *xlink_dev = (struct xlink_ipc_dev *)fd;

	if (!xlink_dev)
		return -ENODEV;

	/* stop the vpu */
	rc = intel_keembay_vpu_stop(xlink_dev->vpu_dev);
	if (rc) {
		pr_err("Failed to stop VPU: %d\n", rc);
		return -EBUSY;
	}
	pr_info("Successfully stopped VPU!\n");

	/* check state */
	state = intel_keembay_vpu_status(xlink_dev->vpu_dev);
	if (state != KEEMBAY_VPU_OFF) {
		pr_err("VPU was not OFF after stop request, it was %d\n", state);
		return -EBUSY;
	}
	return 0;
}
EXPORT_SYMBOL(xlink_ipc_reset_device);

/*
 * xlink_ipc_open_channel() - platform open channel interface.
 *
 * @fd:			[in] The xlink ipc operation descriptor.
 * @channel:	[in] The channnel id to open.
 *
 * Return: 0 on success, negative error code otherwise.
 */
int xlink_ipc_open_channel(void *fd, u32 channel)
{
	int rc = 0;
	struct xlink_ipc_dev *xlink_dev;
	struct xlink_ipc_fd *ipc = (struct xlink_ipc_fd *)fd;

	if (!ipc)
		return -EINVAL;

	xlink_dev = get_xlink_dev_by_sw_device_id(ipc->sw_device_id);
	if (!xlink_dev)
		return -ENODEV;

	rc = intel_keembay_vpu_ipc_open_channel(xlink_dev->vpu_dev, ipc->node,
			channel);
	return rc;
}
EXPORT_SYMBOL(xlink_ipc_open_channel);

/*
 * xlink_ipc_close_channel() - platform close channel interface.
 *
 * @fd:			[in] The xlink ipc operation descriptor.
 * @channel:	[in] The channnel id to close.
 *
 * Return: 0 on success, negative error code otherwise.
 */
int xlink_ipc_close_channel(void *fd, u32 channel)
{
	int rc = 0;
	struct xlink_ipc_dev *xlink_dev;
	struct xlink_ipc_fd *ipc = (struct xlink_ipc_fd *)fd;

	if (!ipc)
		return -EINVAL;

	xlink_dev = get_xlink_dev_by_sw_device_id(ipc->sw_device_id);
	if (!xlink_dev)
		return -ENODEV;

	rc = intel_keembay_vpu_ipc_close_channel(xlink_dev->vpu_dev, ipc->node,
			channel);
	return rc;
}
EXPORT_SYMBOL(xlink_ipc_close_channel);

/*
 * xlink ipc driver functions
 */

static int keembay_xlink_ipc_probe(struct platform_device *pdev)
{
	int rc = 0;
	struct xlink_ipc_dev *xlink_dev;
	struct device *dev = &pdev->dev;

	/* allocate per-device data structure */
	xlink_dev = kzalloc(sizeof(*xlink_dev), GFP_KERNEL);
	if (!xlink_dev)
		return -ENOMEM;

	xlink_dev->pdev = pdev;
	dev_info(dev, "Keem Bay xlink IPC driver probed.\n");

	/* grab reserved memory regions and assign to child devices */
	rc = xlink_reserved_memory_init(xlink_dev);
	if (rc < 0) {
		dev_err(&pdev->dev,
				"Failed to set up reserved memory regions.\n");
		goto r_cleanup;
	}

	/* allocate memory from the reserved memory regions */
	rc = xlink_reserved_memory_alloc(xlink_dev);
	if (rc < 0) {
		dev_err(&pdev->dev,
				"Failed to allocate reserved memory regions.\n");
		goto r_cleanup;
	}

	/* init the xlink buffer pool used for rx/tx */
	init_xlink_buf_pool(xlink_dev);

	/* get reference to vpu ipc device */
	rc = vpu_ipc_device_get(xlink_dev);
	if (rc)
		goto r_cleanup;

	/* get device id */
	rc = of_property_read_u32(dev->of_node, "intel,keembay-vpu-ipc-id",
			&xlink_dev->vpu_id);
	if (rc) {
		/* only warn for now; we will enforce this in the future */
		dev_warn(dev, "VPU ID not defined in DT, using %u as default.\n",
				VPU_SLICE_0_ID);
		dev_warn(dev, "WARNING: additional VPU devices may fail probing.\n");
		xlink_dev->vpu_id = VPU_SLICE_0_ID;
		xlink_dev->sw_device_id = VPU_SLICE_0_SW_DEVICE_ID;
	}

	/* assign a sw device id */
	xlink_dev->sw_device_id = GET_SW_DEVICE_ID_FROM_VPU_ID(xlink_dev->vpu_id);

	/* assign a device name */
	rc = of_property_read_string(dev->of_node, "intel,keembay-vpu-ipc-name",
			&xlink_dev->device_name);
	if (rc) {
		/* only warn for now; we will enforce this in the future */
		dev_warn(dev, "VPU name not defined in DT, using %s as default.\n",
				VPU_SLICE_0_DEVICE_NAME);
		dev_warn(dev, "WARNING: additional VPU devices may fail probing.\n");
		xlink_dev->device_name = VPU_SLICE_0_DEVICE_NAME;
	}

	/* get platform data reference */
	platform_set_drvdata(pdev, xlink_dev);

	/*
	 * to avoid xLink without multi-slice support to fail, we keep track of
	 * vpu with id 0 for now
	 * TODO: remove this once xLink multi-slice is supported
	 */
	if (xlink_dev->vpu_id == 0) {
		if (xlink0_dev) {
			dev_err(dev, "VPU device with ID 0 already probed\n");
		}
		xlink0_dev = xlink_dev;
	}

	dev_info(dev, "Device id=%u sw_device_id=0x%x name=%s probe complete.\n",
			xlink_dev->vpu_id, xlink_dev->sw_device_id,
			xlink_dev->device_name);
	return 0;

r_cleanup:
	xlink_reserved_memory_remove(xlink_dev);
	return rc;
}

static int keembay_xlink_ipc_remove(struct platform_device *pdev)
{
	struct xlink_ipc_dev *xlink_dev = platform_get_drvdata(pdev);
	struct device *dev = &pdev->dev;

	if (xlink_dev->vpu_id == 0)
		xlink0_dev = NULL;

	/*
	 * no need to de-alloc xlink mem (local_xlink_mem and remote_xlink_mem)
	 * since it was allocated with dmam_alloc
	 */
	xlink_reserved_memory_remove(xlink_dev);

	/* release vpu ipc device */
	vpu_ipc_device_put(xlink_dev);

	dev_info(dev, "Keem Bay xlink IPC driver removed.\n");
	return 0;
}

static const struct of_device_id keembay_xlink_ipc_of_match[] = {
	{
		.compatible = "intel,keembay-xlink-ipc",
	},
	{}
};

static struct platform_driver keembay_xlink_ipc_driver = {
	.driver = {
			.name = "keembay-xlink-ipc",
			.of_match_table = keembay_xlink_ipc_of_match,
		},
	.probe = keembay_xlink_ipc_probe,
	.remove = keembay_xlink_ipc_remove,
};
module_platform_driver(keembay_xlink_ipc_driver);

/*
 * get_xlink_dev_by_sw_device_id() - Get xlink_ipc_dev struct by sw device id.
 *
 * @sw_device_id:	[in] The xlink ipc device's sw device id.
 *
 * Return: The xlink ipc device on success, NULL otherwise.
 */
static struct xlink_ipc_dev *get_xlink_dev_by_sw_device_id(u32 sw_device_id)
{
	struct device *dev = NULL;
	struct platform_device *pdev;
	struct xlink_ipc_dev *xlink_dev;

	while ((dev = driver_find_next_device(&keembay_xlink_ipc_driver.driver,
			dev))) {
		pdev = to_platform_device(dev);
		xlink_dev = platform_get_drvdata(pdev);
		if (xlink_dev->sw_device_id == sw_device_id) {
			break;
		} else {
			xlink_dev = NULL;
		}
	}
	return xlink_dev;
}

/*
 * get_xlink_dev_by_name() - Get an xlink_ipc_dev struct by the device name.
 *
 * @device_name:	[in] The xlink ipc device's name.
 *
 * Return: The xlink ipc device on success, NULL otherwise.
 */
static struct xlink_ipc_dev *get_xlink_dev_by_name(const char *device_name)
{
	struct device *dev = NULL;
	struct platform_device *pdev;
	struct xlink_ipc_dev *xlink_dev;

	while ((dev = driver_find_next_device(&keembay_xlink_ipc_driver.driver,
			dev))) {
		pdev = to_platform_device(dev);
		xlink_dev = platform_get_drvdata(pdev);
		if (!strncmp(xlink_dev->device_name, device_name,
				XLINK_IPC_MAX_DEVICE_NAME_SIZE)) {
			break;
		} else {
			xlink_dev = NULL;
		}
	}
	return xlink_dev;
}

MODULE_DESCRIPTION("Keem Bay xlink IPC Driver");
MODULE_AUTHOR("Ryan Carnaghi <ryan.r.carnaghi@intel.com>");
MODULE_LICENSE("Dual BSD/GPL");
