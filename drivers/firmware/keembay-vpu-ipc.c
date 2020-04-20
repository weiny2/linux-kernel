// SPDX-License-Identifier: GPL-2.0-only OR BSD-3-Clause
/*
 * Keem Bay VPU IPC Driver.
 *
 * Copyright (c) 2018-2019 Intel Corporation.
 */

#include <linux/arm-smccc.h>
#include <linux/clk.h>
#include <linux/dma-direct.h>
#include <linux/dma-mapping.h>
#include <linux/firmware.h>
#include <linux/interrupt.h>
#include <linux/io.h>
#include <linux/keembay-ipc.h>
#include <linux/keembay-vpu-ipc.h>
#include <linux/kernel.h>
#include <linux/kthread.h>
#include <linux/module.h>
#include <linux/of.h>
#include <linux/of_address.h>
#include <linux/of_platform.h>
#include <linux/of_reserved_mem.h>
#include <linux/platform_device.h>

/* Function ID for the SiP SMC to boot the VPU */
#define KMB_SIP_SVC_VPU_BOOT (0xFF10)

/* Function ID for the SiP SMC to reset the VPU */
#define KMB_SIP_SVC_VPU_RESET (0xFF15)

/* Function ID for the SiP SMC to stop the VPU */
#define KMB_SIP_SVC_VPU_STOP (0xFF16)

/* Device tree "memory-region" for VPU firmware area */
#define VPU_IPC_FW_AREA_IDX (0)

/* Device tree region for VPU driver to store X509 region */
#define VPU_IPC_X509_AREA_IDX (1)

/* Device tree "memory-region" for MSS IPC header area */
#define MSS_IPC_AREA_IDX (2)

/*
 * These are the parameters of the ready message to be received
 * from the VPU when it is booted correctly.
 */
#define READY_MESSAGE_IPC_CHANNEL (0xA)

/* Ready message timeout, in ms */
/*
 * TODO: we are using 30 secs because the FPGA is slow booting the VPU; change
 * this to 2 secs once switched to silicon.
 */
#define READY_MESSAGE_TIMEOUT_MS (30000)

/* Ready message 'physical data address', which is actually a command. */
#define READY_MESSAGE_EXPECTED_PADDR (0x424f4f54)

/* Ready message size */
#define READY_MESSAGE_EXPECTED_SIZE (0)

/* Size of version information in the header */
#define VPU_VERSION_SIZE (32)

/* Version of header that this driver supports. */
#define HEADER_VERSION_SUPPORTED (0x1)

/* Maximum size allowed for firmware version region */
#define MAX_FIRMWARE_VERSION_SIZE (0x1000)

/* Size allowed for header region */
#define MAX_HEADER_SIZE (0x1000)

/* VPU reset vector must be aligned to 4kB. */
#define VPU_RESET_VECTOR_ALIGNMENT (0x1000)

/* Watchdog timer reset trigger */
#define TIM_WATCHDOG (0x0)

/* Watchdog counter enable register */
#define TIM_WDOG_EN (0x8)

/* Write access to protected registers */
#define TIM_SAFE (0xC)

/* Watchdog timer count value */
#define TIM_WATCHDOG_RESET_VALUE (0xFFFFFFFF)

/* Watchdog timer safe value */
#define TIM_SAFE_ENABLE (0xf1d0dead)

/* Watchdog timeout interrupt clear bit */
#define TIM_GEN_CONFIG_WDOG_TO_INT_CLR BIT(9)

/* Magic number for the boot parameters. */
#define BOOT_PARAMS_MAGIC_NUMBER (0x00010000)

/* Maximum size of string of form "pll_i_out_j" */
#define PLL_STRING_SIZE (128)

/* Number of PLLs */
#define NUM_PLLS (3)

/* Every PLL has this many outputs. */
#define NUM_PLL_OUTPUTS (4)

/**
 * struct boot_parameters
 * @magic_number:		Magic number to indicate structure populated
 * @reserved_0:			Reserved memory for other 'header' information
 * @cpu_frequency_hz:		Frequency that the CPU is running at
 * @pll0_out:			Frequency of each of the outputs of PLL 0
 * @pll1_out:			Frequency of each of the outputs of PLL 1
 * @pll2_out:			Frequency of each of the outputs of PLL 2
 * @reserved_1:			Reserved memory for other clock frequencies
 * @mss_ipc_header_address:	Base address of MSS IPC header region
 * @mss_ipc_header_area_size:	Size of MSS IPC header region
 * @reserved_2:			Reserved memory for other memory regions
 * @mss_wdt_to_irq_a53_redir:	MSS redirects WDT TO IRQ to this ARM IRQ number
 * @nce_wdt_to_irq_a53_redir:	NCE redirects WDT TO IRQ to this ARM IRQ number
 * @reserved_3:			Reserved memory for other configurations
 * @reserved_4:			Unused/reserved memory
 */
struct boot_parameters {
	/* Header: 0x0 - 0x1F */
	uint32_t magic_number;
	uint32_t reserved_0[7];
	/* Clock frequencies: 0x20 - 0xFF */
	uint32_t cpu_frequency_hz;
	uint32_t pll0_out[NUM_PLL_OUTPUTS];
	uint32_t pll1_out[NUM_PLL_OUTPUTS];
	uint32_t pll2_out[NUM_PLL_OUTPUTS];
	uint32_t reserved_1[43];
	/* Memory regions: 0x100 - 0x1FF */
	uint64_t mss_ipc_header_address;
	uint32_t mss_ipc_header_area_size;
	uint32_t reserved_2[61];
	/* IRQ re-direct numbers: 0x200 - 0x2ff */
	uint32_t mss_wdt_to_irq_a53_redir;
	uint32_t nce_wdt_to_irq_a53_redir;
	uint32_t reserved_3[62];
	/* Unused/reserved: 0x300 - 0xFFF */
	uint32_t reserved_4[832];
} __packed;

/**
 * struct firmware_header - Firmware header information
 */
struct firmware_header {
	uint32_t header_version;
	uint32_t image_format;
	uint64_t image_load_address;
	uint32_t image_size;
	uint64_t entry_point;
	uint8_t vpu_version[VPU_VERSION_SIZE];
	uint32_t compression_type;
	uint64_t firmware_version_load_address;
	uint32_t firmware_version_size;
	uint64_t configuration_data_load_address;
} __packed;


/**
 * struct vpu_mem - Information about reserved memory shared with VPU.
 * @vaddr:	The virtual address of the memory region.
 * @paddr:	The (CPU) physical address of the memory region.
 * @vpu_adr:	The VPU address of the memory region.
 * @size:	The size of the memory region.
 */
struct vpu_mem {
	void *vaddr;
	phys_addr_t paddr;
	dma_addr_t vpu_addr;
	size_t size;
};

/**
 * struct vpu_mem - Information about reserved memory shared with ATF.
 * @vaddr:	The virtual address of the memory region.
 * @paddr:	The physical address of the memory region.
 * @size:	The size of the memory region.
 */
struct atf_mem {
	void *vaddr;
	phys_addr_t paddr;
	size_t size;
};

struct vpu_ipc_dev {
	struct platform_device *pdev;
	enum intel_keembay_vpu_state state;
	struct vpu_mem reserved_mem;
	struct atf_mem x509_mem;
	struct vpu_mem mss_ipc_mem;
	uint64_t boot_vec_paddr;
	struct boot_parameters *boot_params;
	struct resource fw_res;
	struct task_struct *ready_message_task;
	spinlock_t lock;
	struct clk *cpu_clock;
	struct clk *pll[NUM_PLLS][NUM_PLL_OUTPUTS];
	int nce_irq;
	int mss_irq;
	uint32_t nce_wdt_redirect;
	uint32_t mss_wdt_redirect;
	uint32_t imr;
	u32 vpu_id;
	void __iomem *nce_wdt_reg;
	void __iomem *nce_tim_cfg_reg;
	void __iomem *mss_wdt_reg;
	void __iomem *mss_tim_cfg_reg;
	unsigned int nce_wdt_count;
	unsigned int mss_wdt_count;
	wait_queue_head_t ready_queue;
	struct device *ipc_dev;
};

enum keembay_vpu_event {
	KEEMBAY_VPU_EVENT_BOOT = 0,
	KEEMBAY_VPU_EVENT_BOOT_FAILED,
	KEEMBAY_VPU_EVENT_RESET,
	KEEMBAY_VPU_EVENT_STOP,
	KEEMBAY_VPU_EVENT_STOP_COMPLETE,
	KEEMBAY_VPU_EVENT_NCE_WDT_TIMEOUT,
	KEEMBAY_VPU_EVENT_MSS_WDT_TIMEOUT,
	KEEMBAY_VPU_EVENT_MSS_READY_OK,
	KEEMBAY_VPU_EVENT_MSS_READY_FAIL
};

static struct vpu_ipc_dev *to_vpu_dev(struct device *dev);

static struct vpu_ipc_dev *vpu0_dev;

/**
 * vpu_ipc_handle_event() - Handle events and optionally update state
 *
 * @vpu_dev:		Private data
 * @event:		Event that has occurred
 *
 * This function is called in the case that an event has occurred. This
 * function tells the user if the event is valid for the current state.
 *
 * This function can be called from interrupt context.
 *
 * Returns: 0 for success otherwise negative error value
 */
static int vpu_ipc_handle_event(struct vpu_ipc_dev *vpu_dev,
				enum keembay_vpu_event event)
{
	unsigned long flags;
	int rc = -EINVAL;
	struct device *dev = &vpu_dev->pdev->dev;

	/* Atomic update of state */
	spin_lock_irqsave(&vpu_dev->lock, flags);

	switch (vpu_dev->state) {
	case KEEMBAY_VPU_OFF:
		if (event == KEEMBAY_VPU_EVENT_BOOT) {
			vpu_dev->state = KEEMBAY_VPU_BUSY;
			rc = 0;
		}
		break;
	case KEEMBAY_VPU_BUSY:
		if (event == KEEMBAY_VPU_EVENT_MSS_READY_OK) {
			vpu_dev->state = KEEMBAY_VPU_READY;
			rc = 0;
		}
		if (event == KEEMBAY_VPU_EVENT_MSS_READY_FAIL ||
		    event == KEEMBAY_VPU_EVENT_BOOT_FAILED) {
			vpu_dev->state = KEEMBAY_VPU_ERROR;
			rc = 0;
		}
		break;
	case KEEMBAY_VPU_READY:
		if (event == KEEMBAY_VPU_EVENT_NCE_WDT_TIMEOUT ||
		    event == KEEMBAY_VPU_EVENT_MSS_WDT_TIMEOUT) {
			vpu_dev->state = KEEMBAY_VPU_ERROR;
			rc = 0;
		}
	/* fallthrough */
	case KEEMBAY_VPU_ERROR:
		if (event == KEEMBAY_VPU_EVENT_RESET) {
			vpu_dev->state = KEEMBAY_VPU_BUSY;
			rc = 0;
		}
		if (event == KEEMBAY_VPU_EVENT_STOP) {
			vpu_dev->state = KEEMBAY_VPU_STOPPING;
			rc = 0;
		}
		break;
	case KEEMBAY_VPU_STOPPING:
		if (event == KEEMBAY_VPU_EVENT_STOP_COMPLETE) {
			vpu_dev->state = KEEMBAY_VPU_OFF;
			rc = 0;
		}
		break;
	default:
		break;
	}

	spin_unlock_irqrestore(&vpu_dev->lock, flags);

	if (rc)
		dev_err(dev, "Can't handle event %d in state %d\n",
				event, vpu_dev->state);

	return rc;
}

static void clear_and_disable_vpu_wdt(void __iomem *wdt_base,
				      void __iomem *tim_cfg_base)
{
	u32 tim_gen_config;

	/* Enable writing and set non-zero WDT value */
	iowrite32(TIM_SAFE_ENABLE, wdt_base + TIM_SAFE);
	iowrite32(TIM_WATCHDOG_RESET_VALUE, wdt_base + TIM_WATCHDOG);

	/* Enable writing and disable watchdog timer */
	iowrite32(TIM_SAFE_ENABLE, wdt_base + TIM_SAFE);
	iowrite32(0, wdt_base + TIM_WDOG_EN);

	/* Now clear the timeout interrupt */
	tim_gen_config = ioread32(tim_cfg_base);
	tim_gen_config &= ~(TIM_GEN_CONFIG_WDOG_TO_INT_CLR);
	iowrite32(tim_gen_config, tim_cfg_base);
}

static irqreturn_t nce_wdt_irq_handler(int irq, void *ptr)
{
	struct vpu_ipc_dev *vpu_dev = ptr;
	struct device *dev = &vpu_dev->pdev->dev;
	int rc;

	clear_and_disable_vpu_wdt(vpu_dev->nce_wdt_reg,
				  vpu_dev->nce_tim_cfg_reg);

	rc = vpu_ipc_handle_event(vpu_dev, KEEMBAY_VPU_EVENT_NCE_WDT_TIMEOUT);
	if (rc < 0)
		dev_err(dev, "Unexpected NCE WDT event.\n");

	vpu_dev->nce_wdt_count++;

	return IRQ_HANDLED;
}

static irqreturn_t mss_wdt_irq_handler(int irq, void *ptr)
{
	struct vpu_ipc_dev *vpu_dev = ptr;
	struct device *dev = &vpu_dev->pdev->dev;
	int rc;

	clear_and_disable_vpu_wdt(vpu_dev->mss_wdt_reg,
				  vpu_dev->mss_tim_cfg_reg);

	rc = vpu_ipc_handle_event(vpu_dev, KEEMBAY_VPU_EVENT_MSS_WDT_TIMEOUT);
	if (rc < 0)
		dev_err(dev, "Unexpected MSS WDT event.\n");

	vpu_dev->mss_wdt_count++;

	return IRQ_HANDLED;
}

static resource_size_t get_reserved_mem_size(struct device *dev,
					     unsigned int idx)
{
	struct resource mem;
	struct device_node *np;
	int rc;

	np = of_parse_phandle(dev->of_node, "memory-region", idx);
	if (!np) {
		pr_err("Couldn't find memory-region %d\n", idx);
		return 0;
	}

	rc = of_address_to_resource(np, 0, &mem);
	if (rc) {
		pr_err("Couldn't map address to resource\n");
		return 0;
	}

	return resource_size(&mem);
}

static int setup_vpu_fw_region(struct vpu_ipc_dev *vpu_dev)
{
	struct device *dev = &vpu_dev->pdev->dev;
	struct vpu_mem *rsvd_mem = &vpu_dev->reserved_mem;
	int rc;

	rc = of_reserved_mem_device_init(dev);
	if (rc) {
		dev_err(dev, "Failed to initialise device reserved memory.\n");
		return rc;
	}

	rsvd_mem->size = get_reserved_mem_size(dev, VPU_IPC_FW_AREA_IDX);
	if (rsvd_mem->size == 0) {
		dev_err(dev, "Couldn't get size of reserved memory region.\n");
		rc = -ENODEV;
		goto setup_vpu_fw_fail;
	}

	rsvd_mem->vaddr = dmam_alloc_coherent(dev, rsvd_mem->size,
					      &rsvd_mem->vpu_addr, GFP_KERNEL);
	/* Get the physical address of the reserved memory region. */
	rsvd_mem->paddr = dma_to_phys(dev, vpu_dev->reserved_mem.vpu_addr);

	if (!rsvd_mem->vaddr) {
		dev_err(dev, "Failed to allocate memory for firmware.\n");
		rc = -ENOMEM;
		goto setup_vpu_fw_fail;
	}

	dev_info(dev, "Memory region (firmware): vpu_addr 0x%pad size 0x%zX\n",
		 &rsvd_mem->vpu_addr, rsvd_mem->size);

	return 0;

setup_vpu_fw_fail:
	of_reserved_mem_device_release(dev);

	return rc;
}

static int setup_x509_region(struct vpu_ipc_dev *vpu_dev)
{
	struct device *dev = &vpu_dev->pdev->dev;
	struct device_node *node;
	struct resource res;
	int rc;

	node = of_parse_phandle(dev->of_node, "memory-region",
				VPU_IPC_X509_AREA_IDX);
	if (!node) {
		dev_err(dev, "Couldn't find X509 region.\n");
		return -EINVAL;
	}

	rc = of_address_to_resource(node, 0, &res);

	/* Release node first as we will not use it anymore */
	of_node_put(node);

	if (rc) {
		dev_err(dev, "Couldn't resolve X509 region.\n");
		return rc;
	}

	vpu_dev->x509_mem.vaddr =
		devm_ioremap(dev, res.start, resource_size(&res));
	if (!vpu_dev->x509_mem.vaddr) {
		dev_err(dev, "Couldn't ioremap x509 region.\n");
		return -EADDRNOTAVAIL;
	}

	vpu_dev->x509_mem.paddr = res.start;
	vpu_dev->x509_mem.size = resource_size(&res);

	dev_info(dev, "Memory region (X509) paddr 0x%pad size 0x%zX\n",
		 &vpu_dev->x509_mem.paddr, vpu_dev->x509_mem.size);

	return 0;
}

static int setup_mss_ipc_region(struct vpu_ipc_dev *vpu_dev)
{
	struct device *dev = &vpu_dev->pdev->dev;
	struct device_node *node;
	struct resource res;
	int rc;

	node = of_parse_phandle(dev->of_node, "memory-region",
				MSS_IPC_AREA_IDX);
	if (!node) {
		dev_err(dev, "Didn't find MSS IPC region.\n");
		return -EINVAL;
	}

	rc = of_address_to_resource(node, 0, &res);
	if (rc) {
		dev_err(dev, "Couldn't resolve MSS IPC region.\n");
		return rc;
	}
	of_node_put(node);

	vpu_dev->mss_ipc_mem.paddr = res.start;
	vpu_dev->mss_ipc_mem.vpu_addr = phys_to_dma(dev, res.start);
	vpu_dev->mss_ipc_mem.size = resource_size(&res);

	dev_info(dev,
		 "Memory region (MSS IPC) vpu_addr %pad paddr %pa size 0x%zX\n",
		 &vpu_dev->mss_ipc_mem.vpu_addr, &vpu_dev->mss_ipc_mem.paddr,
		 vpu_dev->mss_ipc_mem.size);

	return 0;
}

static int setup_reserved_memory(struct vpu_ipc_dev *vpu_dev)
{
	struct device *dev = &vpu_dev->pdev->dev;
	int rc;

	/*
	 * Find the VPU firmware area described in the device tree,
	 * and allocate it for our usage.
	 */
	rc = setup_vpu_fw_region(vpu_dev);
	if (rc) {
		dev_err(dev, "Failed to init FW memory.\n");
		return rc;
	}

	/*
	 * Find the X509 area described in the device tree,
	 * and allocate it for our usage.
	 */
	rc = setup_x509_region(vpu_dev);
	if (rc) {
		dev_err(dev, "Failed to setup X509 region.\n");
		goto res_mem_setup_fail;
	}

	/*
	 * Find the MSS IPC area in the device tree and get the location and
	 * size information
	 */
	rc = setup_mss_ipc_region(vpu_dev);
	if (rc) {
		dev_err(dev, "Couldn't setup MSS IPC region.\n");
		goto res_mem_setup_fail;
	}

	return 0;

res_mem_setup_fail:
	of_reserved_mem_device_release(dev);

	return rc;
}

static void ipc_device_put(struct vpu_ipc_dev *vpu_dev)
{
	put_device(vpu_dev->ipc_dev);
}

static int ipc_device_get(struct vpu_ipc_dev *vpu_dev)
{
	struct device *dev = &vpu_dev->pdev->dev;
	struct device_node *np;
	struct platform_device *pdev;

	np = of_parse_phandle(dev->of_node, "intel,keembay-ipc", 0);
	if (!np) {
		dev_warn(dev, "Cannot find phandle to IPC device\n");
		/*
		 * For (temporary) retro-compatibility, if the IPC phandle is
		 * not defined, we try to find an IPC device whose DT node is a
		 * sibling of the VPU node.
		 */
		dev_warn(dev, "Fallback: looking for a sibling IPC node\n");
		np = of_get_compatible_child(dev->parent->of_node,
					     "intel,keembay-ipc");
		if (!np) {
			dev_err(dev, "Couldn't find a sibling IPC node\n");
			return -ENODEV;
		}
	}

	pdev = of_find_device_by_node(np);
	if (!pdev) {
		dev_info(dev, "IPC device not probed\n");
		of_node_put(np);
		return -EPROBE_DEFER;
	}

	vpu_dev->ipc_dev = get_device(&pdev->dev);
	of_node_put(np);

	dev_info(dev, "Using IPC device: %s\n", dev_name(vpu_dev->ipc_dev));

	return 0;
}

static int retrieve_clocks(struct vpu_ipc_dev *vpu_dev)
{
	struct device *dev = &vpu_dev->pdev->dev;
	struct clk *clk;
	int pll = 0;
	int out = 0;
	char pll_string[PLL_STRING_SIZE];

	clk = devm_clk_get(dev, "cpu_clock");
	if (IS_ERR(clk)) {
		dev_err(dev, " cpu_clock not found.\n");
		return PTR_ERR(clk);
	}
	vpu_dev->cpu_clock = clk;

	for (pll = 0; pll < NUM_PLLS; pll++) {
		for (out = 0; out < NUM_PLL_OUTPUTS; out++) {
			sprintf(pll_string, "pll_%d_out_%d", pll, out);
			clk = devm_clk_get(dev, pll_string);
			if (IS_ERR(clk)) {
				dev_err(dev, " %s not found.\n", pll_string);
				return PTR_ERR(clk);
			}
			vpu_dev->pll[pll][out] = clk;
		}
	}

	return 0;
}

static int get_pdev_res_and_ioremap(struct platform_device *pdev,
				    const char *reg_name,
				    void __iomem **target_reg)
{
	struct resource *res;
	void __iomem *reg;
	struct device *dev = &pdev->dev;

	res = platform_get_resource_byname(pdev, IORESOURCE_MEM, reg_name);
	if (!res) {
		dev_err(dev, "Couldn't get resource for %s\n", reg_name);
		return -EINVAL;
	}

	reg = devm_ioremap_resource(dev, res);
	if (IS_ERR(reg)) {
		dev_err(dev, "Couldn't ioremap resource for %s\n", reg_name);
		return PTR_ERR(reg);
	}
	dev_info(dev, "Register base for %s: 0x%pa\n", reg_name, &res->start);

	*target_reg = reg;

	return 0;
}

static int setup_watchdog_resources(struct vpu_ipc_dev *vpu_dev)
{
	int rc;
	struct device *dev = &vpu_dev->pdev->dev;
	struct platform_device *pdev = vpu_dev->pdev;

	/* Get registers */
	rc = get_pdev_res_and_ioremap(pdev, "nce_wdt", &vpu_dev->nce_wdt_reg);
	if (rc) {
		dev_err(dev, "Failed to get NCE WDT registers\n");
		return rc;
	}
	rc = get_pdev_res_and_ioremap(pdev, "nce_tim_cfg",
				      &vpu_dev->nce_tim_cfg_reg);
	if (rc) {
		dev_err(dev, "Failed to get NCE TIM_GEN_CONFIG register\n");
		return rc;
	}
	rc = get_pdev_res_and_ioremap(pdev, "mss_wdt", &vpu_dev->mss_wdt_reg);
	if (rc) {
		dev_err(dev, "Failed to get MSS WDT registers\n");
		return rc;
	}
	rc = get_pdev_res_and_ioremap(pdev, "mss_tim_cfg",
				      &vpu_dev->mss_tim_cfg_reg);
	if (rc) {
		dev_err(dev, "Failed to get MSS TIM_GEN_CONFIG register\n");
		return rc;
	}

	/* Request interrupts */
	vpu_dev->nce_irq = platform_get_irq_byname(pdev, "nce_wdt");
	if (vpu_dev->nce_irq < 0) {
		dev_err(dev, "failed to retrieve NCE IRQ.\n");
		return vpu_dev->nce_irq;
	}
	vpu_dev->mss_irq = platform_get_irq_byname(pdev, "mss_wdt");
	if (vpu_dev->mss_irq < 0) {
		dev_err(dev, "failed to retrieve MSS IRQ.\n");
		return vpu_dev->mss_irq;
	}
	rc = devm_request_irq(dev, vpu_dev->nce_irq, nce_wdt_irq_handler, 0,
			      "keembay-vpu-ipc", vpu_dev);
	if (rc) {
		dev_err(dev, "failed to request NCE IRQ.\n");
		return rc;
	}
	rc = devm_request_irq(dev, vpu_dev->mss_irq, mss_wdt_irq_handler, 0,
			      "keembay-vpu-ipc", vpu_dev);
	if (rc) {
		dev_err(dev, "failed to request MSS IRQ.\n");
		return rc;
	}

	/* Request interrupt re-direct numbers */
	rc = of_property_read_u32(dev->of_node,
				  "intel,keembay-vpu-ipc-nce-wdt-redirect",
				  &vpu_dev->nce_wdt_redirect);
	if (rc) {
		dev_err(dev, "failed to get NCE WDT redirect number.\n");
		return rc;
	}
	rc = of_property_read_u32(dev->of_node,
				  "intel,keembay-vpu-ipc-mss-wdt-redirect",
				  &vpu_dev->mss_wdt_redirect);
	if (rc) {
		dev_err(dev, "failed to get MSS WDT redirect number.\n");
		return rc;
	}

	return 0;
}

static int setup_boot_parameters(struct vpu_ipc_dev *vpu_dev)
{
	int i;

	/* Set all values to zero. This will disable most clocks/devices */
	memset(vpu_dev->boot_params, 0, sizeof(*vpu_dev->boot_params));

	/*
	 * Set magic number, so VPU knows that the parameters are
	 * populated correctly
	 */
	vpu_dev->boot_params->magic_number = BOOT_PARAMS_MAGIC_NUMBER;

	/* Inform VPU of clock frequencies */
	vpu_dev->boot_params->cpu_frequency_hz =
		clk_get_rate(vpu_dev->cpu_clock);
	for (i = 0; i < NUM_PLL_OUTPUTS; i++) {
		vpu_dev->boot_params->pll0_out[i] =
			clk_get_rate(vpu_dev->pll[0][i]);
		vpu_dev->boot_params->pll1_out[i] =
			clk_get_rate(vpu_dev->pll[1][i]);
		vpu_dev->boot_params->pll2_out[i] =
			clk_get_rate(vpu_dev->pll[2][i]);
	}

	/* Fill in IPC buffer information */
	vpu_dev->boot_params->mss_ipc_header_address =
		vpu_dev->mss_ipc_mem.vpu_addr;
	vpu_dev->boot_params->mss_ipc_header_area_size =
		vpu_dev->mss_ipc_mem.size;

	/* Fill in re-direct request information */
	vpu_dev->boot_params->mss_wdt_to_irq_a53_redir =
		vpu_dev->mss_wdt_redirect;
	vpu_dev->boot_params->nce_wdt_to_irq_a53_redir =
		vpu_dev->nce_wdt_redirect;

	return 0;
}

static int request_vpu_boot(struct vpu_ipc_dev *vpu_dev)
{
	uint64_t function_id;
	uint16_t function_number = KMB_SIP_SVC_VPU_BOOT;
	struct arm_smccc_res res;

	function_id = ARM_SMCCC_CALL_VAL(ARM_SMCCC_FAST_CALL, ARM_SMCCC_SMC_32,
					 ARM_SMCCC_OWNER_SIP, function_number);

	/* Arguments are as follows:
	 * 1. VPU ID (was 'Reserved region physical base address')
	 * 2. Reserved region size
	 * 3. Firmware image physical base address
	 * 4. Firmware image size
	 * 5. Entry point for VPU
	 * 6. X509 certificate location
	 * 7. IMR driver number
	 *
	 * FIXME: fw_res addresses are VPU addresses; convert them to CPU
	 * physical addresses.
	 *
	 * FIXME: Parameter 'Reserved region physical base address' was removed
	 * by IO team when they added multi-slice support in ATF. This needs to
	 * be re-added somehow.
	 */
	arm_smccc_smc(function_id, vpu_dev->vpu_id,
		      vpu_dev->reserved_mem.size, vpu_dev->fw_res.start,
		      resource_size(&vpu_dev->fw_res), vpu_dev->boot_vec_paddr,
		      vpu_dev->x509_mem.paddr, vpu_dev->imr, &res);

	if (res.a0) {
		dev_info(&vpu_dev->pdev->dev, "Boot failed: 0x%lx.\n", res.a0);
		return -EIO;
	}

	dev_info(&vpu_dev->pdev->dev, "Boot SMC successful.\n");

	return 0;
}

static int request_vpu_reset(struct vpu_ipc_dev *vpu_dev)
{
	uint64_t function_id;
	uint16_t function_number = KMB_SIP_SVC_VPU_RESET;
	struct arm_smccc_res res;

	function_id = ARM_SMCCC_CALL_VAL(ARM_SMCCC_FAST_CALL, ARM_SMCCC_SMC_32,
					 ARM_SMCCC_OWNER_SIP, function_number);

	arm_smccc_smc(function_id, vpu_dev->vpu_id, 0, 0, 0, 0, 0, 0, &res);

	if (res.a0) {
		dev_info(&vpu_dev->pdev->dev, "Reset failed: 0x%lx.\n", res.a0);
		return -EIO;
	}

	dev_info(&vpu_dev->pdev->dev, "Reset SMC successful.\n");

	return 0;
}

static int request_vpu_stop(struct vpu_ipc_dev *vpu_dev)
{
	uint64_t function_id;
	uint16_t function_number = KMB_SIP_SVC_VPU_STOP;
	struct arm_smccc_res res;

	function_id = ARM_SMCCC_CALL_VAL(ARM_SMCCC_FAST_CALL, ARM_SMCCC_SMC_32,
					 ARM_SMCCC_OWNER_SIP, function_number);

	arm_smccc_smc(function_id, vpu_dev->vpu_id, 0, 0, 0, 0, 0, 0, &res);

	if (res.a0) {
		dev_info(&vpu_dev->pdev->dev, "Stop failed: 0x%lx.\n", res.a0);
		return -EIO;
	}

	dev_info(&vpu_dev->pdev->dev, "Stop SMC successful.\n");

	return 0;
}

static void *get_vpu_dev_vaddr(struct vpu_ipc_dev *vpu_dev,
			       struct resource *res)
{
	unsigned long offset = 0;

	/* Given the calculation below, this must not be true. */
	if (res->start < vpu_dev->reserved_mem.vpu_addr)
		return NULL;

	offset = res->start - vpu_dev->reserved_mem.vpu_addr;

	return vpu_dev->reserved_mem.vaddr + offset;
}

static int parse_fw_header(struct vpu_ipc_dev *vpu_dev,
			   const struct firmware *fw)
{
	struct firmware_header *fw_header = NULL;
	struct resource config_res, version_res, total_reserved_res;
	void *fw_region = NULL;
	void *version_region = NULL;
	void *config_region = NULL;
	struct device *dev = &vpu_dev->pdev->dev;

	/* Is the fw size big enough to read the header? */
	if (fw->size < sizeof(struct firmware_header)) {
		dev_err(dev, "Firmware was too small for header.\n");
		return -EINVAL;
	}

	fw_header = (struct firmware_header *)fw->data;

	/* Check header version */
	if (fw_header->header_version != HEADER_VERSION_SUPPORTED) {
		dev_err(dev, "Header version check expected 0x%x, got 0x%x\n",
			HEADER_VERSION_SUPPORTED, fw_header->header_version);
		return -EINVAL;
	}

	/* Check firmware version size is allowed */
	if (fw_header->firmware_version_size > MAX_FIRMWARE_VERSION_SIZE) {
		dev_err(dev, "Firmware version area larger than allowed: %d\n",
			fw_header->firmware_version_size);
		return -EINVAL;
	}

	/*
	 * Check the firmware binary is at least large enough for the
	 * firmware image size described in the header.
	 */
	if (fw->size < (MAX_HEADER_SIZE + MAX_FIRMWARE_VERSION_SIZE +
			fw_header->image_size)) {
		dev_err(dev, "Real firmware size is not large enough.\n");
		return -EINVAL;
	}

	/*
	 * Make sure that the final address is aligned correctly. If not, the
	 * boot will never work.
	 */
	if (!IS_ALIGNED(fw_header->entry_point, VPU_RESET_VECTOR_ALIGNMENT)) {
		dev_err(dev,
			"Entry point for firmware (0x%llX) is not correctly aligned.\n",
			fw_header->entry_point);
		return -EINVAL;
	}

	/*
	 * Generate the resource describing the region containing the actual
	 * firmware data.
	 */
	vpu_dev->fw_res.start = fw_header->image_load_address;
	vpu_dev->fw_res.end =
		fw_header->image_size + fw_header->image_load_address - 1;
	vpu_dev->fw_res.flags = IORESOURCE_MEM;

	/*
	 * Generate the resource describing the region containing the
	 * configuration data for the VPU.
	 */
	config_res.start = fw_header->configuration_data_load_address;
	config_res.end = sizeof(struct boot_parameters) +
			 fw_header->configuration_data_load_address - 1;
	config_res.flags = IORESOURCE_MEM;

	/*
	 * Generate the resource describing the region containing the
	 * version information for the VPU.
	 */
	version_res.start = fw_header->firmware_version_load_address;
	version_res.end = fw_header->firmware_version_size +
			  fw_header->firmware_version_load_address - 1;
	version_res.flags = IORESOURCE_MEM;

	/*
	 * Generate the resource describing the region of memory
	 * completely dedicated to the VPU.
	 */
	total_reserved_res.start = vpu_dev->reserved_mem.vpu_addr;
	total_reserved_res.end = vpu_dev->reserved_mem.vpu_addr +
		vpu_dev->reserved_mem.size - 1;
	total_reserved_res.flags = IORESOURCE_MEM;

	/*
	 * Check all pieces to be copied reside completely in the reserved
	 * region
	 */
	if (!resource_contains(&total_reserved_res, &vpu_dev->fw_res)) {
		dev_err(dev, "Can't fit firmware in reserved region.\n");
		return -EINVAL;
	}
	if (!resource_contains(&total_reserved_res, &version_res)) {
		dev_err(dev,
			"Can't fit firmware version data in reserved region.\n");
		return -EINVAL;
	}
	if (!resource_contains(&total_reserved_res, &config_res)) {
		dev_err(dev,
			"Can't fit configuration information in reserved region.\n");
		return -EINVAL;
	}

	/* Check for overlapping regions */
	if (resource_overlaps(&vpu_dev->fw_res, &version_res)) {
		dev_err(dev, "FW and version regions overlap.\n");
		return -EINVAL;
	}
	if (resource_overlaps(&vpu_dev->fw_res, &config_res)) {
		dev_err(dev, "FW and config regions overlap.\n");
		return -EINVAL;
	}
	if (resource_overlaps(&config_res, &version_res)) {
		dev_err(dev, "Version and config regions overlap.\n");
		return -EINVAL;
	}

	/* Setup boot parameter region */
	config_region = get_vpu_dev_vaddr(vpu_dev, &config_res);
	if (!config_region) {
		dev_err(dev,
			"Couldn't map boot configuration area to CPU virtual address.\n");
		return -EINVAL;
	}
	version_region = get_vpu_dev_vaddr(vpu_dev, &version_res);
	if (!version_region) {
		dev_err(dev,
			"Couldn't map version area to CPU virtual address.\n");
		return -EINVAL;
	}
	fw_region = get_vpu_dev_vaddr(vpu_dev, &vpu_dev->fw_res);
	if (!fw_region) {
		dev_err(dev,
			"Couldn't map firmware area to CPU virtual address.\n");
		return -EINVAL;
	}

	/*
	 * Copy version region: the region is located in the file @ offset of
	 * MAX_HEADER_SIZE, size was specified in the header and has been
	 * checked to not be larger than that allowed.
	 */
	memcpy(version_region, &fw->data[MAX_HEADER_SIZE],
	       fw_header->firmware_version_size);

	/*
	 * Copy firmware region: the region is located in the file @ offset of
	 * MAX_HEADER_SIZE + MAX_FIRMWARE_VERSION_SIZE, size was specified in
	 * the header and has been checked to not be larger than that allowed.
	 */
	memcpy(fw_region,
	       &fw->data[MAX_HEADER_SIZE + MAX_FIRMWARE_VERSION_SIZE],
	       fw_header->image_size);

	/* Save off boot parameters region vaddr */
	vpu_dev->boot_params = (struct boot_parameters *)config_region;

	/* Save off boot vector physical address */
	vpu_dev->boot_vec_paddr = fw_header->entry_point;

	/* TODO: handle X.509 region */

	return 0;
}

static int ready_message_wait_thread(void *arg)
{
	struct vpu_ipc_dev *vpu_dev = (struct vpu_ipc_dev *)arg;
	struct device *dev = &vpu_dev->pdev->dev;
	int rc = 0;
	int close_rc = 0;
	uint32_t paddr = 0;
	size_t size = 0;

	/*
	 * We will wait a few seconds for the message. We will
	 * complete earlier if the message is received earlier.
	 */
	rc = intel_keembay_ipc_recv(vpu_dev->ipc_dev, KMB_IPC_NODE_LEON_MSS,
				    READY_MESSAGE_IPC_CHANNEL, &paddr, &size,
				    READY_MESSAGE_TIMEOUT_MS);
	if (rc < 0) {
		dev_err(dev,
			"Failed to receive ready message inside %d ms: %d.\n",
			READY_MESSAGE_TIMEOUT_MS, rc);
	}

	/* Close the channel. */
	close_rc = intel_keembay_ipc_close_channel(vpu_dev->ipc_dev,
						   KMB_IPC_NODE_LEON_MSS,
						   READY_MESSAGE_IPC_CHANNEL);
	if (close_rc < 0)
		dev_warn(dev, "Couldn't close IPC channel.\n");

	if (rc)
		goto ready_message_thread_failure;

	if (paddr != READY_MESSAGE_EXPECTED_PADDR ||
	    size != READY_MESSAGE_EXPECTED_SIZE) {
		dev_err(dev, "Bad ready message: (paddr, size) = (0x%x, %zu)\n",
			paddr, size);
		goto ready_message_thread_failure;
	}

	dev_info(dev, "VPU ready message received successfully!\n");

	rc = vpu_ipc_handle_event(vpu_dev, KEEMBAY_VPU_EVENT_MSS_READY_OK);
	if (rc < 0)
		dev_err(dev, "Fatal error: failed to set state (ready ok).\n");

	/* Wake up anyone waiting for READY. */
	wake_up_all(&vpu_dev->ready_queue);

	return 0;

ready_message_thread_failure:
	rc = vpu_ipc_handle_event(vpu_dev, KEEMBAY_VPU_EVENT_MSS_READY_FAIL);
	if (rc < 0)
		dev_err(dev,
			"Fatal error: failed to set state (ready timeout).\n");

	return 0;
}

static int create_ready_message_thread(struct vpu_ipc_dev *vpu_dev)
{
	struct task_struct *task;
	struct device *dev = &vpu_dev->pdev->dev;

	task = kthread_run(&ready_message_wait_thread, (void *)vpu_dev,
			   "keembay-vpu-ipc-ready");
	if (IS_ERR(task)) {
		dev_err(dev, "Couldn't start thread to receive message.\n");
		return -EIO;
	}

	vpu_dev->ready_message_task = task;

	return 0;
}

static int kickoff_vpu_sequence(struct vpu_ipc_dev *vpu_dev,
				int (*boot_fn)(struct vpu_ipc_dev *))
{
	int rc;
	int error_close_rc;
	struct device *dev = &vpu_dev->pdev->dev;

	/*
	 * OK, ready to start the parsing/copying process. We now
	 * flush the IPC driver state.
	 *
	 * TODO: flush IPC state
	 */

	/*
	 * Open the IPC channel. If we don't do it before booting
	 * the VPU, we may miss the message, as the IPC driver will
	 * discard messages for unopened channels.
	 */
	rc = intel_keembay_ipc_open_channel(vpu_dev->ipc_dev,
					    KMB_IPC_NODE_LEON_MSS,
					    READY_MESSAGE_IPC_CHANNEL);
	if (rc < 0) {
		dev_err(dev,
			"Couldn't open IPC channel to receive ready message.\n");
		goto kickoff_failed;
	}

	/* Request boot */
	rc = boot_fn(vpu_dev);
	if (rc < 0) {
		dev_err(dev, "Failed to do request to boot.\n");
		goto close_and_kickoff_failed;
	}

	/*
	 * Start thread waiting for message, and update state
	 * if the request was successful.
	 */
	rc = create_ready_message_thread(vpu_dev);
	if (rc < 0) {
		dev_err(dev,
			"Failed to create thread to wait for ready message.\n");
		goto close_and_kickoff_failed;
	}

	return 0;

close_and_kickoff_failed:
	/* Close the channel. */
	error_close_rc = intel_keembay_ipc_close_channel(
				vpu_dev->ipc_dev, KMB_IPC_NODE_LEON_MSS,
				READY_MESSAGE_IPC_CHANNEL);
	if (error_close_rc < 0) {
		dev_err(dev, "Couldn't close IPC channel.\n");
		/*
		 * We have had a more serious failure - don't update the
		 * original 'rc' and fall through.
		 */
	}

kickoff_failed:
	return rc;
}

int intel_keembay_vpu_ipc_open_channel(struct device *dev, u8 node_id,
				       u16 chan_id)
{
	struct vpu_ipc_dev *vpu_dev = to_vpu_dev(dev);

	if (IS_ERR(vpu_dev))
		return -EINVAL;

	return intel_keembay_ipc_open_channel(vpu_dev->ipc_dev, node_id,
					      chan_id);
}
EXPORT_SYMBOL(intel_keembay_vpu_ipc_open_channel);

int intel_keembay_vpu_ipc_close_channel(struct device *dev, u8 node_id,
					u16 chan_id)
{
	struct vpu_ipc_dev *vpu_dev = to_vpu_dev(dev);

	if (IS_ERR(vpu_dev))
		return -EINVAL;

	return intel_keembay_ipc_close_channel(vpu_dev->ipc_dev,
					       node_id, chan_id);
}
EXPORT_SYMBOL(intel_keembay_vpu_ipc_close_channel);

int intel_keembay_vpu_ipc_send(struct device *dev, u8 node_id, u16 chan_id,
			       u32 vpu_addr, size_t size)
{
	struct vpu_ipc_dev *vpu_dev = to_vpu_dev(dev);

	if (IS_ERR(vpu_dev))
		return -EINVAL;

	return intel_keembay_ipc_send(vpu_dev->ipc_dev, node_id, chan_id,
				      vpu_addr, size);
}
EXPORT_SYMBOL(intel_keembay_vpu_ipc_send);

int intel_keembay_vpu_ipc_recv(struct device *dev, u8 node_id, u16 chan_id,
			       u32 *paddr, size_t *size, u32 timeout)
{
	struct vpu_ipc_dev *vpu_dev = to_vpu_dev(dev);

	if (IS_ERR(vpu_dev))
		return -EINVAL;

	return intel_keembay_ipc_recv(vpu_dev->ipc_dev, node_id, chan_id, paddr,
				      size, timeout);
}
EXPORT_SYMBOL(intel_keembay_vpu_ipc_recv);

/**
 * intel_keembay_vpu_startup() - Boot the VPU
 * @firmware_name: Name of firmware file
 *
 * This API is only valid while the VPU is OFF.
 *
 * The firmware called "firmware_name" will be searched for using the
 * kernel firmware API. The firmware header will then be parsed. This driver
 * will load requested information to the reserved memory region, including
 * initialisation data. Lastly, we will request the secure world to do the
 * boot sequence. If the boot sequence is successful, the
 * VPU state will become BUSY. The caller should then wait for the status to
 * become READY before starting to communicate with the VPU. If the boot
 * sequence failed, this function will fail and the caller may try again,
 * the VPU status will still be OFF.
 *
 * If we fail to get to READY, because the VPU did not send us the 'ready'
 * message, the VPU state will go to ERROR.
 *
 * Return: 0 on success, negative error code otherwise
 */
int intel_keembay_vpu_startup(struct device *dev, const char *firmware_name)
{
	int rc;
	int event_rc;
	const struct firmware *fw;
	struct vpu_ipc_dev *vpu_dev = to_vpu_dev(dev);

	if (IS_ERR(vpu_dev))
		return -EINVAL;

	rc = vpu_ipc_handle_event(vpu_dev, KEEMBAY_VPU_EVENT_BOOT);
	if (rc < 0) {
		dev_err(dev, "Can't start in this state.\n");
		return rc;
	}

	dev_info(dev, "Keem Bay VPU IPC start with %s.\n", firmware_name);

	/* Request firmware and wait for it. */
	rc = request_firmware(&fw, firmware_name, &vpu_dev->pdev->dev);
	if (rc < 0) {
		dev_err(dev, "Couldn't find firmware: %d\n", rc);
		goto boot_failed_no_fw;
	}

	/* Do checks on the firmware header. */
	rc = parse_fw_header(vpu_dev, fw);
	if (rc < 0) {
		dev_err(dev, "Firmware checks failed.\n");
		goto boot_failed;
	}

	/* Write configuration data. */
	rc = setup_boot_parameters(vpu_dev);
	if (rc < 0) {
		dev_err(dev, "Failed to set up boot parameters.\n");
		goto boot_failed;
	}

	/* Try 'boot' sequence */
	rc = kickoff_vpu_sequence(vpu_dev, request_vpu_boot);
	if (rc < 0) {
		dev_err(dev, "Failed to boot VPU.\n");
		goto boot_failed;
	}

	release_firmware(fw);
	return 0;

boot_failed:
	release_firmware(fw);

boot_failed_no_fw:
	event_rc = vpu_ipc_handle_event(vpu_dev,
					KEEMBAY_VPU_EVENT_BOOT_FAILED);
	if (event_rc < 0)
		dev_err(dev, "Fatal error: failed to handle fail event: %d.\n",
				event_rc);

	/* Prefer original 'rc' to the 'event_rc'. */

	return rc;
}
EXPORT_SYMBOL(intel_keembay_vpu_startup);

/**
 * intel_keembay_vpu_reset() - Reset the VPU
 *
 * Resets the VPU. Only valid when the VPU is in the READY or ERROR state.
 * The state of the VPU will become BUSY.
 *
 * Return: 0 on success, negative error code otherwise
 */
int intel_keembay_vpu_reset(struct device *dev)
{
	int rc;
	int event_rc;
	struct vpu_ipc_dev *vpu_dev = to_vpu_dev(dev);

	if (IS_ERR(vpu_dev))
		return -EINVAL;

	rc = vpu_ipc_handle_event(vpu_dev, KEEMBAY_VPU_EVENT_RESET);
	if (rc < 0) {
		dev_err(dev, "Can't reset in this state.\n");
		return rc;
	}

	dev_info(dev, "Keem Bay VPU IPC reset.\n");

	/* Try 'reset' sequence */
	rc = kickoff_vpu_sequence(vpu_dev, request_vpu_reset);
	if (rc < 0) {
		dev_err(dev, "Failed to reset VPU.\n");
		goto reset_failed;
	}

	return 0;

reset_failed:
	/* Reset failed somewhere, reset the state. */
	event_rc = vpu_ipc_handle_event(vpu_dev, KEEMBAY_VPU_EVENT_BOOT_FAILED);
	if (event_rc < 0)
		dev_err(dev, "Fatal error: failed to handle fail event: %d.\n",
				event_rc);

	/* Prefer original 'rc' to the 'event_rc'. */

	return rc;
}
EXPORT_SYMBOL(intel_keembay_vpu_reset);

/**
 * intel_keembay_vpu_stop() - Stop the VPU
 *
 * Stops the VPU and restores to the OFF state. Only valid when the VPU is in
 * the READY or ERROR state.
 *
 * Return: 0 on success, negative error code otherwise
 */
int intel_keembay_vpu_stop(struct device *dev)
{
	int rc;
	int event_rc;
	struct vpu_ipc_dev *vpu_dev = to_vpu_dev(dev);

	if (IS_ERR(vpu_dev))
		return -EINVAL;

	rc = vpu_ipc_handle_event(vpu_dev, KEEMBAY_VPU_EVENT_STOP);
	if (rc < 0) {
		dev_err(dev, "Can't stop in this state.\n");
		return rc;
	}

	dev_info(dev, "Keem Bay VPU IPC stop.\n");

	/* Request stop */
	rc = request_vpu_stop(vpu_dev);
	if (rc < 0) {
		dev_err(dev,
			"Failed to do request to stop - resetting state to OFF anyway.\n");
	}

	event_rc = vpu_ipc_handle_event(vpu_dev,
					KEEMBAY_VPU_EVENT_STOP_COMPLETE);
	if (event_rc < 0) {
		dev_err(dev,
			"Failed to handle 'stop complete' event, probably fatal.\n");
		return event_rc;
	}

	return rc;
}
EXPORT_SYMBOL(intel_keembay_vpu_stop);

/**
 * intel_keembay_vpu_status() - Get the VPU state.
 *
 * Returns the state of the VPU as tracked by this driver.
 *
 * Return: Relevant value of enum intel_keembay_vpu_state
 */
enum intel_keembay_vpu_state intel_keembay_vpu_status(struct device *dev)
{
	struct vpu_ipc_dev *vpu_dev = to_vpu_dev(dev);

	if (IS_ERR(vpu_dev))
		return -EINVAL;

	return vpu_dev->state;
}
EXPORT_SYMBOL(intel_keembay_vpu_status);

/**
 * intel_keembay_vpu_get_wdt_count() - Get the WDT count
 * @id:		ID of WDT events we wish to get
 *
 * Returns: Number of WDT timeout occurrences for given ID, or negative
 *	    error value for invalid ID.
 */
int intel_keembay_vpu_get_wdt_count(struct device *dev,
				    enum intel_keembay_wdt_cpu_id id)
{
	int rc = -EINVAL;
	struct vpu_ipc_dev *vpu_dev = to_vpu_dev(dev);

	if (IS_ERR(vpu_dev))
		return -EINVAL;

	switch (id) {
	case KEEMBAY_VPU_NCE:
		rc = vpu_dev->nce_wdt_count;
		break;
	case KEEMBAY_VPU_MSS:
		rc = vpu_dev->mss_wdt_count;
		break;
	default:
		break;
	}
	return rc;
}
EXPORT_SYMBOL(intel_keembay_vpu_get_wdt_count);

/**
 * intel_keembay_vpu_wait_for_ready() - Sleep until VPU is READY
 * @timeout:	How long (in ms) the function will block waiting for the VPU
 *		to become ready.
 *
 * The caller may ask the VPU IPC driver to notify it when the VPU
 * is READY. The driver performs no checks on the current state, so it
 * is up to the caller to confirm that the state is correct before starting
 * the wait.
 *
 * Returns: 0 on success negative error code otherwise
 */
int intel_keembay_vpu_wait_for_ready(struct device *dev, u32 timeout)
{
	int rc;
	struct vpu_ipc_dev *vpu_dev = to_vpu_dev(dev);

	if (IS_ERR(vpu_dev))
		return -EINVAL;

	/*
	 * If we are in ERROR state, we will not get to READY
	 * state without some other transitions, so return
	 * error immediately for caller to handle.
	 */
	if (vpu_dev->state == KEEMBAY_VPU_ERROR)
		return -EIO;

	rc = wait_event_interruptible_timeout(
			vpu_dev->ready_queue,
			vpu_dev->state == KEEMBAY_VPU_READY,
			msecs_to_jiffies(timeout));

	/* Condition was false after timeout elapsed */
	if (!rc)
		rc = -ETIME;

	/* Condition was true, so rc == 1 */
	if (rc > 0)
		rc = 0;

	return rc;
}
EXPORT_SYMBOL(intel_keembay_vpu_wait_for_ready);

static int keembay_vpu_ipc_probe(struct platform_device *pdev)
{
	int rc;
	struct vpu_ipc_dev *vpu_dev;
	struct device *dev = &pdev->dev;

	dev_info(dev, "Keem Bay VPU IPC driver probed.\n");

	vpu_dev = devm_kzalloc(dev, sizeof(*vpu_dev), GFP_KERNEL);
	if (!vpu_dev)
		return -ENOMEM;

	vpu_dev->pdev = pdev;
	vpu_dev->state = KEEMBAY_VPU_OFF;
	vpu_dev->ready_message_task = NULL;
	vpu_dev->nce_wdt_count = 0;
	vpu_dev->mss_wdt_count = 0;
	spin_lock_init(&vpu_dev->lock);
	init_waitqueue_head(&vpu_dev->ready_queue);

	/* Retrieve clocks */
	rc = retrieve_clocks(vpu_dev);
	if (rc) {
		dev_err(dev, "Failed to retrieve clocks %d\n", rc);
		return rc;
	}

	rc = ipc_device_get(vpu_dev);
	if (rc)
		return rc;

	/* Retrieve memory regions, allocate memory */
	rc = setup_reserved_memory(vpu_dev);
	if (rc) {
		dev_err(dev, "Failed to set up reserved memory regions: %d\n",
			rc);
		return rc;
	}

	/* Request watchdog timer resources */
	rc = setup_watchdog_resources(vpu_dev);
	if (rc) {
		dev_err(dev, "Failed to setup watchdog resources %d\n", rc);
		goto probe_fail_post_resmem_setup;
	}

	/* Request the IMR number to be used */
	rc = of_property_read_u32(dev->of_node, "intel,keembay-vpu-ipc-imr",
				  &vpu_dev->imr);
	if (rc) {
		dev_err(dev, "failed to get IMR number.\n");
		goto probe_fail_post_resmem_setup;
	}

	/* Get VPU ID. */
	rc = of_property_read_u32(dev->of_node, "intel,keembay-vpu-ipc-id",
				  &vpu_dev->vpu_id);
	if (rc) {
		/* Only warn for now; we will enforce this in the future. */
		dev_warn(dev,
			 "VPU ID not defined in Device Tree, using 0 as default.\n");
		dev_warn(dev,
			 "WARNING: This may cause additional VPU devices to fail probing.\n");
		vpu_dev->vpu_id = 0;
	}

	/* Set platform data reference. */
	platform_set_drvdata(pdev, vpu_dev);

	/*
	 * To avoid xLink without multi-slice support to fail, we keep track of
	 * VPU with ID 0 for now.
	 * TODO: remove this once xLink is supported.
	 */
	if (vpu_dev->vpu_id == 0) {
		if (vpu0_dev) {
			dev_err(dev, "VPU device with ID 0 already probed\n");
			goto probe_fail_post_resmem_setup;
		}
		vpu0_dev = vpu_dev;
	}

	return 0;

probe_fail_post_resmem_setup:
	of_reserved_mem_device_release(dev);

	return rc;
}

static int keembay_vpu_ipc_remove(struct platform_device *pdev)
{
	struct vpu_ipc_dev *vpu_dev = platform_get_drvdata(pdev);
	struct device *dev = &pdev->dev;

	if (vpu_dev->vpu_id == 0)
		vpu0_dev = NULL;

	if (vpu_dev->ready_message_task) {
		kthread_stop(vpu_dev->ready_message_task);
		vpu_dev->ready_message_task = NULL;
	}

	of_reserved_mem_device_release(dev);

	ipc_device_put(vpu_dev);

	dev_info(dev, "Keem Bay VPU IPC driver removed.\n");

	return 0;
}

static const struct of_device_id keembay_vpu_ipc_of_match[] = {
	{
		.compatible = "intel,keembay-vpu-ipc",
	},
	{}
};

static struct platform_driver keem_bay_vpu_ipc_driver = {
	.driver = {
			.name = "keembay-vpu-ipc",
			.of_match_table = keembay_vpu_ipc_of_match,
		},
	.probe = keembay_vpu_ipc_probe,
	.remove = keembay_vpu_ipc_remove,
};
module_platform_driver(keem_bay_vpu_ipc_driver);

/*
 * Helper function to get a vpu_dev struct from a generic device pointer.
 */
static struct vpu_ipc_dev *to_vpu_dev(struct device *dev)
{
	struct platform_device *pdev;

	/*
	 * For temporary compatibility with xLink, if dev is NULL, we return
	 * the VPU with ID 0.
	 * TODO: remove this once xLink is updated.
	 */
	if (!dev && vpu0_dev)
		return vpu0_dev;

	if (!dev || dev->driver != &keem_bay_vpu_ipc_driver.driver)
		return ERR_PTR(-EINVAL);
	pdev = to_platform_device(dev);

	return platform_get_drvdata(pdev);
}

MODULE_DESCRIPTION("Keem Bay VPU IPC Driver");
MODULE_AUTHOR("Paul Murphy <paul.j.murphy@intel.com>");
MODULE_AUTHOR("Daniele Alessandrelli <daniele.alessandrelli@intel.com>");
MODULE_LICENSE("Dual BSD/GPL");
