/*
 * SVFS ACPI Driver for Linux
 *
 * Copyright (C) 2010 Intel Corp.
 *
 * Based on skeleton from the drivers/char/efirtc.c driver by S. Eranian
 *
 */

#include <linux/types.h>
#include <linux/errno.h>
#include <linux/miscdevice.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/proc_fs.h>
#include <linux/seq_file.h>
#include <linux/version.h>
#include <linux/acpi.h>
#include "../acpi/acpica/accommon.h"
#include "../acpi/acpica/aclocal.h"
#include "../acpi/acpica/acmacros.h"
#include "../acpi/acpica/acutils.h"
#include "../acpi/acpica/acstruct.h"
#include "../acpi/acpica/acobject.h"
#include "../acpi/acpica/acnamesp.h"
#include <acpi/acoutput.h>
#define _COMPONENT   0x00000400
static const char _acpi_module_name[] = "svfs-acpi";
#include <linux/pci.h>
#include <linux/efi.h>
#include "svfs_acpi.h"
#include <asm/uaccess.h>
#include <asm/mpspec.h>
extern acpi_status acpi_ut_evaluate_object(struct acpi_namespace_node *prefix_node,
         const char *path,
         u32 expected_return_btypes,
         union acpi_operand_object **return_desc);

#define SVFS_ACPI_VERSION		"1.04"

#define STRUCT_TO_INT(s)		(*((int*)&s))

static DEFINE_SPINLOCK(svfs_acpi_lock);

static long svfs_acpi_ioctl(struct file *file, unsigned int cmd, unsigned long arg);

typedef struct {
	acpi_handle	obj;
	acpi_handle	parent;
	unsigned char devfn;
	short busno;
	short segno;
} svfs_bus_hier_t;

#define MAX_PCI_BUSES	(MAX_MP_BUSSES - 1)	// one bus for ISA
static svfs_bus_hier_t	svfs_bus_hier[MAX_PCI_BUSES];

static svfs_irq_src_t	svfs_irq_src[MAX_IRQ_SOURCES];
static int	svfs_irq_src_index = 0;

static struct {
	acpi_handle obj;
	unsigned int pirq_irq;
} svfs_pirq[MAX_PIRQ];

static int svfs_num_pirq = 0;

static	acpi_handle		svfs_sysbus;	/* Handle to "\_SB_" */
#define SVFS_PRT_BUF_SIZE	4096
static unsigned char 	svfs_prt_buf[SVFS_PRT_BUF_SIZE];

#define acpi_ns_map_handle_to_node(handle)  ACPI_CAST_PTR(struct acpi_namespace_node, handle)
typedef union acpi_object acpi_object;
typedef struct acpi_object_list acpi_object_list;
typedef struct acpi_buffer acpi_buffer;
typedef struct acpi_device_info acpi_device_info;
typedef struct acpi_namespace_node acpi_namespace_node;
typedef union acpi_operand_object acpi_operand_object;
typedef struct acpi_pci_routing_table acpi_pci_routing_table;

static acpi_status svfs_parse_prt(acpi_handle, u32, void *, void **);

static void
svfs_acpi_prep(void)
{
	int	bus;

		// set/reset local stuff
	for (bus = 0 ; (bus < MAX_PCI_BUSES) && svfs_bus_hier[bus].obj; bus++)
		svfs_bus_hier[bus].obj = 0;
	svfs_irq_src_index = 0;

		// get handle to "system bus" so can walk ACPI namespace
	if (ACPI_FAILURE(acpi_get_handle(0, "\\_SB_", &svfs_sysbus))) {
		printk(KERN_WARNING "%s: failed to get \"\\_SB_\" handle!\n", __FUNCTION__ );
	} 
}

static void 
svfs_acpi_pic(int mode)
{
	acpi_object acpi_myobj;
	acpi_object_list acpi_mylist;
	acpi_buffer acpi_mybuf;
	char retbuf[512];
	acpi_status status;

		// execute \_PIC(1|2) for SYM mode or \_PIC(0) for VWx mode */
	acpi_myobj.integer.type = ACPI_TYPE_INTEGER;
	acpi_myobj.integer.value = mode;
	acpi_mylist.count = 1;
	acpi_mylist.pointer = &acpi_myobj;
	acpi_mybuf.length = sizeof(retbuf);

	status = acpi_evaluate_object(NULL, "\\_PIC", &acpi_mylist, NULL);
	if (ACPI_FAILURE(status)) {
		printk(KERN_WARNING "%s: \\PIC(%ld) failed,  status %d\n", __FUNCTION__, (unsigned long)acpi_myobj.integer.value, status);
	}
}

static int
svfs_get_pci_bus(int dev_bus)
{
	int	par_bus;
	struct pci_dev *dev;
	u16 class;
	u8 header_type;

	if(svfs_bus_hier[dev_bus].busno != -1)
		return 0;

	for (par_bus = 0; (par_bus < MAX_PCI_BUSES) && (svfs_bus_hier[par_bus].obj != NULL); par_bus++) {
		if (svfs_bus_hier[par_bus].obj == svfs_bus_hier[dev_bus].parent) {
			u8 sec_bus;
			unsigned tmp_seg, tmp_bus, tmp_devfn;

			if (svfs_bus_hier[par_bus].busno == -1)
				if (svfs_get_pci_bus(par_bus) != 0)	// exit with error
					return -1;

		// w/ parent's bus # and bridge's DF, read bridge's sec bus #
			tmp_bus = svfs_bus_hier[par_bus].busno;
			tmp_seg = svfs_bus_hier[dev_bus].segno;
			tmp_devfn = svfs_bus_hier[dev_bus].devfn;
			printk(KERN_DEBUG "%s: finding PCI dev for %04x:%02x:%02x.%d\n", 
				__FUNCTION__, tmp_seg, tmp_bus, tmp_devfn >> 3, tmp_devfn & 0x07);
			dev = pci_get_domain_bus_and_slot(tmp_seg, tmp_bus, tmp_devfn);
			if (dev == NULL) {
				printk(KERN_DEBUG "%s: can't find PCI dev for %04x:%02x:%02x.%d\n", 
					__FUNCTION__, tmp_seg, tmp_bus, tmp_devfn >> 3, tmp_devfn & 0x07);
				return -1;
			}
		// verify this is a bridge
			pci_read_config_word(dev, PCI_CLASS_DEVICE, &class);
			pci_read_config_byte(dev, PCI_HEADER_TYPE, &header_type);
			header_type &= 0x7f;
			if (class == PCI_CLASS_BRIDGE_PCI && 
					(header_type == PCI_HEADER_TYPE_NORMAL || header_type == PCI_HEADER_TYPE_BRIDGE)) {
				pci_read_config_byte(dev, PCI_SECONDARY_BUS, &sec_bus);
				svfs_bus_hier[dev_bus].busno = sec_bus;
				svfs_bus_hier[dev_bus].segno = tmp_seg;
				printk(KERN_DEBUG "%s: sec bus for %04x:%02x:%02x.%d is %#x\n", 
					__FUNCTION__, tmp_seg, tmp_bus, tmp_devfn >> 3, tmp_devfn & 0x07, sec_bus);
				return 0;
			}
			break;
		}
	}
	return -1;
}

static void 
dumpSVFSdata(void)
{
	int bus, src;
	for (bus = 0; bus < MAX_PCI_BUSES && svfs_bus_hier[bus].obj; bus++) {
		if(svfs_bus_hier[bus].busno == -1) 
			printk(KERN_DEBUG "bus_hier[%d] %04x:-1:%02x.%d\n",bus,
			svfs_bus_hier[bus].segno, 
			svfs_bus_hier[bus].devfn >> 3, svfs_bus_hier[bus].devfn & 0x07);
		else
			printk(KERN_DEBUG "bus_hier[%d] %04x:%02x:%02x.%d\n", bus,
			svfs_bus_hier[bus].segno, svfs_bus_hier[bus].busno,
			svfs_bus_hier[bus].devfn >> 3, svfs_bus_hier[bus].devfn & 0x07);
	}
	for (src = 0; src < svfs_irq_src_index; src++) {
		if(svfs_irq_src[src].busno == -1) 
			printk(KERN_DEBUG "irq_src[%d]  %04x:-1:%02x.%d\n", src,
			svfs_irq_src[src].segno,
			svfs_irq_src[src].device, (signed)svfs_irq_src[src].function);
		else	
			printk(KERN_DEBUG "irq_src[%d]  %04x:%02x:%02x.%d\n", src,
			svfs_irq_src[src].segno, svfs_irq_src[src].busno,
			svfs_irq_src[src].device, (signed)svfs_irq_src[src].function);
	}
}

static int
svfs_acpi_walk_prt(void)
{
	acpi_status status;
	acpi_handle	parent_obj;	// ACPI handle to parent PCI bus' object
	acpi_handle	pirq_obj;	// ACPI handle to device's LNK object
	int bus, src, pirq;
	struct acpi_get_devices_info info;

	do {
		// evaluate ALL the _PRT methods */
	status = acpi_walk_namespace(ACPI_TYPE_METHOD, svfs_sysbus, ACPI_UINT32_MAX, svfs_parse_prt, NULL, &info, NULL);
	if (ACPI_FAILURE(status))
		break;

	for (bus = 0; bus < MAX_PCI_BUSES && svfs_bus_hier[bus].obj; bus++) {
		if (svfs_get_pci_bus(bus) != 0) {
			// printk(KERN_DEBUG "%s: can't enumerate PCI bus %#x\n", __FUNCTION__, bus);
		}
	}
		// update svfs_irq_src[] with device's bus # from svfs_bus_hier[]
	for (src = 0; src < svfs_irq_src_index; src++) {
		parent_obj = svfs_irq_src[src].parent_obj;
			// force to -1 so can detect entries with no defined bus
		svfs_irq_src[src].busno = -1;
		for (bus = 0; bus < MAX_PCI_BUSES && svfs_bus_hier[bus].obj; bus++) {
			if (parent_obj == svfs_bus_hier[bus].obj) {
				svfs_irq_src[src].segno = svfs_bus_hier[bus].segno;
				svfs_irq_src[src].busno = svfs_bus_hier[bus].busno;
				break;
			}
		}
	}
		// update svfs_irq_src[] with device's pirq # from svfs_pirq[]
	for (src = 0; src < svfs_irq_src_index; src++) {
		pirq_obj = svfs_irq_src[src].pirq_obj;
		if (pirq_obj) {
			for (pirq = 0; pirq < MAX_PIRQ; pirq++) {
				if (pirq_obj == svfs_pirq[pirq].obj) {
					svfs_irq_src[src].pirq = pirq + 1;
					svfs_irq_src[src].pirq_irq = svfs_pirq[pirq].pirq_irq;
				}
			}
		}
	}

	}
	while (0);

	return status;
}

static int
svfs_get_acpi_object_name(acpi_handle obj, char *buffer, int len)
{
	acpi_buffer	acpi_buf;
	int status;

	acpi_buf.length = len;
	acpi_buf.pointer = buffer;

	status = acpi_get_name(obj, ACPI_FULL_PATHNAME, &acpi_buf);
	
	if (!ACPI_SUCCESS(status)) {
		printk("%s: can't get ACPI object name\n", __FUNCTION__ );
		status = -1;
	} 
	else
		status = 0;
	return status;
}

static int
svfs_get_acpi_parent(acpi_handle obj, acpi_handle *parent_obj)
{
	int status;

	status = acpi_get_parent(obj, parent_obj);
	if (!ACPI_SUCCESS(status)) {
		printk("%s: acpi_get_parent() failed (status %d)\n", __FUNCTION__, status);
		status = -1;
	} 
	else
		status = 0;
	return status;
}

static acpi_device_info *
svfs_get_acpi_object_info(acpi_handle obj)
{
	int status;
	acpi_device_info *info = NULL;
	status = acpi_get_object_info(obj, &info);
	if (!ACPI_SUCCESS(status))
		printk("%s: acpi_get_object_info() failed (status %d)\n", __FUNCTION__, status);
	return info;
}

static void
svfs_free_acpi_object_info(acpi_device_info *info)
{
	kfree(info);
}

/*
 * Return: 0 on success, nonzero on failure
 */
static int
svfs_set_bus_hier(acpi_handle obj)
{
	int	bus;
	acpi_handle	parent;
	acpi_device_info *devinfo;
	int status = -1;

	for (bus = 0; bus < MAX_PCI_BUSES; bus++) {
		if (svfs_bus_hier[bus].obj == obj)
			return 0;			// already have it!
		else { 
			if (svfs_bus_hier[bus].obj == NULL)
				break;			// not found since at end of list
		}
	}
	if (bus == MAX_PCI_BUSES) {
		printk("%s: > %d PCI buses detected\n", __FUNCTION__, MAX_PCI_BUSES);
		return -1;
	}

	do {
// need to verify obj is PCI bridge or PCI host
		svfs_bus_hier[bus].obj = obj;
		svfs_bus_hier[bus].busno = -1;
		svfs_bus_hier[bus].segno = 0;
		if (svfs_get_acpi_parent(obj, &svfs_bus_hier[bus].parent))
			break;
		/*
	 	* check if object has "._BBN" value, which specifies explicit bus 
	 	* else assume bus 0 for any root PCI bus with \_SB_ parent 
	 	*/

		if (svfs_get_acpi_object_name(obj, svfs_prt_buf, sizeof(svfs_prt_buf)))
			break;

		if (strlen(svfs_prt_buf) >= (SVFS_PRT_BUF_SIZE - 6)) {
			printk("%s: ACPI object name too long: %s\n", __FUNCTION__, svfs_prt_buf);
			break;
		}

		{	// note new scope!
			acpi_namespace_node *obj_node;
			struct acpi_device *device = NULL;
			u64 seg_value;
			u64 bus_value;
			acpi_status	status;

			obj_node = acpi_ns_map_handle_to_node(obj);
       			if (!obj_node) {
				printk("%s(%d): can't map acpi handle to node\n", __FUNCTION__, __LINE__);
				break;
			}

			status = acpi_bus_get_device(svfs_bus_hier[bus].parent, &device);
			if (ACPI_FAILURE(status)) {
				printk("%s: can't get acpi parent bus device for %s\n", __FUNCTION__, svfs_prt_buf);
				break;
			}

    			status = acpi_evaluate_integer(device->handle, METHOD_NAME__SEG, NULL, &seg_value);
    			if (ACPI_SUCCESS(status)) {
				svfs_bus_hier[bus].segno = ACPI_LOWORD(seg_value);
				printk("%s(%d)[%d]: parent PCI segment number 0x%x for %s\n",
				__FUNCTION__, __LINE__,bus,
				svfs_bus_hier[bus].segno, svfs_prt_buf);
			}
			else {
				struct acpi_device *thisdevice = NULL;
				status = acpi_bus_get_device(obj, &thisdevice);
				if (ACPI_SUCCESS(status)) {
				    	status = acpi_evaluate_integer(thisdevice->handle, METHOD_NAME__SEG, NULL, &seg_value);
		  	  		if (ACPI_SUCCESS(status)) {
						svfs_bus_hier[bus].segno = ACPI_LOWORD(seg_value);
						printk("%s(%d)[%d]: PCI segment number 0x%x for %s\n",
						__FUNCTION__, __LINE__,bus,
						svfs_bus_hier[bus].segno, svfs_prt_buf);
					}
					else {
						printk("%s(%d)[%d]: can't get PCI segment number for %s\n",
							__FUNCTION__, __LINE__, bus, svfs_prt_buf);
						svfs_bus_hier[bus].segno = 0;
					}
				} else {
					printk("%s: can't get acpi bus device for %s\n", __FUNCTION__, svfs_prt_buf);
					svfs_bus_hier[bus].segno = 0;
				}
			}

    			status = acpi_ut_evaluate_numeric_object(METHOD_NAME__BBN, obj_node, &bus_value);
    			if (ACPI_SUCCESS(status)) {
				svfs_bus_hier[bus].busno = ACPI_LOWORD(bus_value);
			}
			else {
				if (svfs_bus_hier[bus].parent == svfs_sysbus) {
					svfs_bus_hier[bus].busno = 0;		// default = 0
				}
			}
		}
		status = 0;
	}
	while (0);
	if (status) {
		svfs_bus_hier[bus].obj = 0;
		return -1;
	}
	/* 
	 * if bus # still unknown, use bus' device/function on parent bus;
	 * this is PCI bridge dev/fn, so evaluate object's _ADR method
	 */
	if (svfs_bus_hier[bus].busno == -1) {
		devinfo = svfs_get_acpi_object_info(obj);
		if (!devinfo)
			return -1;

		/* 
		 * ACPI device/function: device [31:16], function [15:0]
		 *  PCI device/function: device [ 7: 3], function [ 2:0]
		 */
		svfs_bus_hier[bus].devfn = ((devinfo->address & 0xffff0000) >> 13) | (devinfo->address & 7);
		svfs_free_acpi_object_info(devinfo);
	}

	/*
	 * add device's parent to hierarchy array if not already included
	 */
	parent = svfs_bus_hier[bus].parent;

	if(parent == svfs_sysbus)
		return 0;

	for (bus = 0; bus < MAX_PCI_BUSES; bus++) {
		if (svfs_bus_hier[bus].obj == parent)
			return 0;	// exit point for recursive svfs_bus_hier()

		if (svfs_bus_hier[bus].obj == NULL)
			break;
	}
	return svfs_set_bus_hier(parent);
}

#define AML_INT_NAMEPATH_OP         (u16) 0x002d

// called for every method found in the \_SB_ tree; parse if "_PRT"
static acpi_status
svfs_parse_prt(acpi_handle obj, u32 level, void *context, void **return_value)
{
	acpi_buffer	acpi_mybuf;
	acpi_status	status;
    acpi_handle parent, pirq_obj;
	acpi_pci_routing_table  prt;
	acpi_operand_object **top_object_list = NULL;
    acpi_operand_object **sub_object_list = NULL;
    acpi_operand_object *package_element = NULL;
	acpi_operand_object *package_object;
    int num_pkg_entries, pkg;
	svfs_irq_src_t *pp;

	struct acpi_device *device = NULL;
	int result;

	/*
	 * Step 1 - retrieve the raw PRT data by evaluating this method
	 */
	acpi_mybuf.length = sizeof(svfs_prt_buf);
	acpi_mybuf.pointer = svfs_prt_buf;

	if (svfs_get_acpi_object_name(obj, svfs_prt_buf, sizeof(svfs_prt_buf)))
		return 0;
	
	if (strcmp("_PRT", &svfs_prt_buf[strlen(svfs_prt_buf) - 4]))
		return 0;

	/* get handle to parent PCI bus object */
	if (svfs_get_acpi_parent(obj, &parent))
		return -1;

		// evaluate status of parent PCI bus 
	status = acpi_bus_get_device(parent, &device);
	if (ACPI_FAILURE(status)) {
		//printk("%s: can't get acpi bus device\n", __FUNCTION__);
		return 0;
	}
	result = acpi_bus_get_status(device);
	if (result) {
		printk("%s: can't read device [%s] status w/%d\n", 
			__FUNCTION__, device->pnp.bus_id, result);
		return 0;
	}
	if (!device->status.present)
		return 0;

	/* try to evaluate the _PRT method */
	acpi_mybuf.length = sizeof(svfs_prt_buf);

	if (svfs_get_acpi_object_name(parent, svfs_prt_buf, sizeof(svfs_prt_buf)))
    	return 0;

	acpi_mybuf.length = sizeof(svfs_prt_buf);
	status = acpi_ut_evaluate_object(parent, METHOD_NAME__PRT, ACPI_BTYPE_PACKAGE, &package_object);

    if (ACPI_FAILURE(status)) {
    	printk("%s(%d)\n", __FUNCTION__, __LINE__);
    	return_ACPI_STATUS(status);
    }
    if (!package_object) { // must return object!
    	printk("%s(%d): no object returned from _PRT\n", __FUNCTION__, __LINE__);
		return -1;
    }
	top_object_list  = package_object->package.elements;
    num_pkg_entries = package_object->package.count;

	pirq_obj = NULL;
    for (pkg = 0; pkg < num_pkg_entries; pkg++) {

		memset(&prt, 0, sizeof(acpi_pci_routing_table));
			// dereference the sub-package
		package_element = *top_object_list;
			// sub_object_list points to array of 4 IRQ elements:
			// Address, Pin, Source and Source_index
		sub_object_list = package_element->package.elements;
		/*
		 * 1) First subobject:  get the Address
		 */
		if (ACPI_TYPE_INTEGER == (*sub_object_list)->common.type) {
			prt.address = (*sub_object_list)->integer.value;
		}
		else {
			printk("%s(%d): need Integer, found %s\n", 
				__FUNCTION__, __LINE__,
				acpi_ut_get_type_name ((*sub_object_list)->common.type));
			return_ACPI_STATUS (AE_BAD_DATA);
		}

		/*
		 * 2) Second subobject: get the Pin
		 */
		sub_object_list++;

		if (ACPI_TYPE_INTEGER == (*sub_object_list)->common.type) {
			prt.pin = (u32) (*sub_object_list)->integer.value;
			// BIOS now can use ZeroOp and OneOp for pin
		}
		else {
			printk("%s(%d): need Integer, found %s\n", 
				__FUNCTION__, __LINE__,
				acpi_ut_get_type_name ((*sub_object_list)->common.type));
			return_ACPI_STATUS (AE_BAD_DATA);
		}

		/*
		 * 3) Third subobject: get the Source Name
		 */
		sub_object_list++;

		switch ((*sub_object_list)->common.type) {
		case ACPI_TYPE_LOCAL_REFERENCE:
			{
				acpi_handle node;
				struct acpi_buffer path;
				path.length = ACPI_ALLOCATE_LOCAL_BUFFER;
				node = (acpi_handle)(*sub_object_list)->reference.node;
				status = acpi_ns_handle_to_pathname(node, &path, TRUE);
				if (ACPI_FAILURE(status)) {
					printk("%s: can't convert name to path\n", __FUNCTION__);
		   			return_ACPI_STATUS (AE_BAD_DATA);
				}
				acpi_get_handle(parent, path.pointer, &pirq_obj);
				ACPI_FREE(path.pointer);
			}
			break;
		case ACPI_TYPE_STRING:
			acpi_get_handle(parent, (*sub_object_list)->string.pointer, &pirq_obj);
			break;
		case ACPI_TYPE_INTEGER:
			break;

		default:
		   	return_ACPI_STATUS (AE_BAD_DATA);
		}

		/*
		 * 4) Fourth subobject: get the Source Index
		 */
		sub_object_list++;

		if (ACPI_TYPE_INTEGER == (*sub_object_list)->common.type) {
			prt.source_index = (u32) (*sub_object_list)->integer.value;
		} else {
			return_ACPI_STATUS (AE_BAD_DATA);
		}
		top_object_list++;

		do {
			u32 device, function, pin;

			device = (prt.address >> 16) & 0xffff;
			function = prt.address & 0xffff;
			pin = prt.pin & 0x03;
				// updated on 2nd pass with PIC(0) for VWx
			if (pirq_obj) {
				int index;
				for (index = 0; index < svfs_irq_src_index; index++) {
					pp = &svfs_irq_src[index];
					if (pp->parent_obj == parent && pp->device == device && 
							pp->function == function && pp->pin == pin) {
						pp->pirq_obj = pirq_obj;
						break;
					}
				}
				break;
			}
			else {
				if (svfs_irq_src_index >= MAX_IRQ_SOURCES) {
			//		printk ("%s: >= %d IRQ sources!\n", __FUNCTION__, MAX_IRQ_SOURCES);
					break;
				}
// don't add if the device is not present, JKT BIOS returns 0 for _BBN() if not present
					/* add the entry */
				pp = &svfs_irq_src[svfs_irq_src_index];

				pp->parent_obj = parent;
				pp->device = device;
				pp->function = function;
				pp->pin = pin;
				pp->gsi = prt.source_index;

				svfs_irq_src_index++;
			}
		}
		while (0);
	}
	acpi_ut_remove_reference(package_object);

		// add this _PRT method's PCI bus (parent) to PCI bus hierarchy 
	if (svfs_set_bus_hier(parent) != 0) {
		printk("%s: can't add parent bus to PCI bus hierarchy\n", __FUNCTION__ );
		return -1;
	}
	return 0;
}

static acpi_status
svfs_acpi_get_current_link(struct acpi_resource *resource, void *context)
{
	int *irq = (int *)context;

	switch (resource->type) {
	case ACPI_RESOURCE_TYPE_IRQ:
		{
			struct acpi_resource_irq *p = &resource->data.irq;
			if (!p || !p->interrupt_count) {
				/*
				 * IRQ descriptors may have no IRQ# bits set,
				 * particularly those those w/ _STA disabled
				 */
				printk("%s: blank IRQ resource\n", __FUNCTION__);
				return AE_OK;
			}
			*irq = p->interrupts[0];
		}
		break;
	case ACPI_RESOURCE_TYPE_EXTENDED_IRQ:
		{
			struct acpi_resource_extended_irq *p = &resource->data.extended_irq;
			if (!p || !p->interrupt_count) {
				/*
				 * extended IRQ descriptors must
				 * return at least 1 IRQ
				 */
				printk("%s: blank EXT IRQ resource\n", __FUNCTION__);
				return AE_OK;
			}
			*irq = p->interrupts[0];
			break;
		}
		break;
	case ACPI_RESOURCE_TYPE_END_TAG:
		return AE_OK;
		break;
	default:
		printk("%s: resource %d isn't an IRQ\n", __FUNCTION__, resource->type);
		break;
	}
	return AE_CTRL_TERMINATE;
}

static acpi_status
svfs_acpi_walk_handler(acpi_handle handle, u32 level, void *dev_dep, void **dev_retv)
{
	acpi_status status;
	char *hardware_id;
	struct acpi_buffer buffer = { ACPI_ALLOCATE_BUFFER, NULL };
	struct acpi_device_info *info;
	int irq;

	status = acpi_get_object_info(handle, &info);
	if (ACPI_FAILURE(status))
		return status;

	do {
		if (!(info->valid & ACPI_VALID_HID)) {
			break;
		}

		hardware_id = info->hardware_id.string;
		if ((hardware_id == NULL) || (strcmp(hardware_id, ACPI_PCI_LINK_HID))) {
			break;
		}

		if (svfs_num_pirq >= MAX_PIRQ) {
			break;
		}
			// let's read the current IRQ assignment
		status = acpi_walk_resources(handle, METHOD_NAME__CRS, svfs_acpi_get_current_link, &irq);
		if (ACPI_FAILURE(status)) {
			printk("%s: can't evaluate LNK[A-H] _CRS method\n", __FUNCTION__ );
			status = -ENODEV;
			break;
		}
// note the match and increment count so can match to it later
		svfs_pirq[svfs_num_pirq].obj = handle;
		svfs_pirq[svfs_num_pirq].pirq_irq = irq;

		svfs_num_pirq++;
	}
	while (0);

	kfree(buffer.pointer);

	return status;
}

#define __GET_USER_DATA(DEST,SRC,SIZE) ({ \
	if (copy_from_user((DEST),(void *)(SRC),(SIZE))) \
		return -EFAULT; \
})

#define GET_USER_DATA(DEST) ({ \
	__GET_USER_DATA(&(DEST),arg,sizeof(DEST)); \
})

#define __PUT_USER_DATA(DEST,SRC,SIZE) ({ \
	if (copy_to_user((void*)(DEST),(void *)(SRC),(SIZE))) \
		return -EFAULT; \
})

#define PUT_USER_DATA(SRC) ({ \
	__PUT_USER_DATA(arg,&(SRC),sizeof(SRC)); \
})

static long
svfs_acpi_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
	int	mode, status = 0;
	picArg_t	picArg;
	unsigned long flags;
	struct acpi_device_info *info;

	if (acpi_disabled || acpi_noirq) {
		printk("%s: acpi_disabled %d, acpi_noirq %d\n", __FUNCTION__, acpi_disabled, acpi_noirq);
		return -ENODEV;
	}

	spin_lock_irqsave(&svfs_acpi_lock, flags);
	switch (cmd) {
		case SVFS_ACPI_READ_IOAPIC:
		{
#define INDEX	0
#define WINDOW	0x10/sizeof(u32)
			rdioapic_t rdioapic;
			u8 *ioapicptr;
			u32 *valptr;

			if (copy_from_user(&rdioapic,(void *)arg,sizeof(rdioapic))) {
				spin_unlock_irqrestore(&svfs_acpi_lock, flags);
				return -EFAULT; 
			}
			ioapicptr = ioremap(rdioapic.full_abar, 0x200);
			if (ioapicptr == NULL) {
				spin_unlock_irqrestore(&svfs_acpi_lock, flags);
				return -ENOMEM; 
			}
			valptr = (u32*)ioapicptr;
			ioapicptr[INDEX] = rdioapic.index;
			rdioapic.value = valptr[WINDOW];
			iounmap(ioapicptr);
			if (copy_to_user((void *)arg,&rdioapic,sizeof(rdioapic))) {
				spin_unlock_irqrestore(&svfs_acpi_lock, flags);
				return -EFAULT; 
			}
		}
			break;

		case SVFS_ACPI_PREP:
			svfs_acpi_prep();
			break;

		case SVFS_ACPI_PIC:
			GET_USER_DATA(picArg);
			mode = picArg.mode;
			svfs_acpi_pic(mode);
			break;

		case SVFS_ACPI_WALK_LNK:
			status = acpi_walk_namespace(ACPI_TYPE_DEVICE, ACPI_ROOT_OBJECT,
					ACPI_UINT32_MAX, svfs_acpi_walk_handler, NULL, &info, NULL);
			if (ACPI_FAILURE(status)) {
				printk("%s: error(%d) walking LNK[A-D] objects\n", __FUNCTION__, status);
				status = -ENODEV;
			}
			break;

		case SVFS_ACPI_WALK_PRT:
			status = svfs_acpi_walk_prt();
			dumpSVFSdata();
			if (ACPI_FAILURE(status)) {
				printk("%s: error(%d) walking PRT objects\n", __FUNCTION__, status);
				status = -ENODEV;
			}
			break;

		case SVFS_ACPI_WALK_SIZE:
			{
			walkSize_t walkSize;

			walkSize.total_bytes = svfs_irq_src_index * sizeof(svfs_irq_src_t);
			walkSize.num_entries = svfs_irq_src_index;
 			PUT_USER_DATA(walkSize);
			}
			break;

		case SVFS_ACPI_WALK_DATA:
			{
			int total_bytes;

			total_bytes = svfs_irq_src_index * sizeof(svfs_irq_src_t);
			if (copy_to_user((void *)arg, (void *)svfs_irq_src, total_bytes) != 0)
				status = -EFAULT;
			}
			break;

		case SVFS_ACPI_GET_RSDP:
			{
			unsigned long rsdp = 0UL;

			if (efi_enabled(EFI_BOOT) && (efi.acpi20 != EFI_INVALID_TABLE_ADDR))
				rsdp = efi.acpi20;
			PUT_USER_DATA(rsdp);
			}
			break;

		default:
			status = -EINVAL;
			break;
	}
	spin_unlock_irqrestore(&svfs_acpi_lock, flags);
	return status;
}

static int
svfs_acpi_open(struct inode *inode, struct file *file)
{
	/*
	 * nothing special to do here
	 * We do accept multiple open files at the same time as we
	 * synchronize on the per call operation.
	 */
	return 0;
}

static int
svfs_acpi_close(struct inode *inode, struct file *file)
{
	return 0;
}

/*
 *	The various file operations we support.
 */

static const struct file_operations svfs_acpi_fops = {
	.owner		= THIS_MODULE,
	.unlocked_ioctl	= svfs_acpi_ioctl,
	.open		= svfs_acpi_open,
	.release	= svfs_acpi_close,
};

static struct miscdevice svfs_acpi_dev=
{
	MISC_DYNAMIC_MINOR,
	"svfs_acpi",
	&svfs_acpi_fops
};

/*
 *	export RAW SVFS ACPI information to /proc/driver/svfs_acpi
 */
static int svfs_acpi_proc_show(struct seq_file *m, void *v)
{
	unsigned long	flags;

	spin_lock_irqsave(&svfs_acpi_lock, flags);

// read/get stuff here

	spin_unlock_irqrestore(&svfs_acpi_lock,flags);

	return 0;
}

static int
svfs_acpi_proc_open(struct inode *inode, struct file *file)
{
	return single_open(file, svfs_acpi_proc_show, NULL);
}

static const struct proc_ops svfs_acpi_proc_ops = {
	.proc_open	= svfs_acpi_proc_open,
	.proc_read	= seq_read,
	.proc_lseek	= seq_lseek,
	.proc_release	= single_release,
};

static int __init 
svfs_acpi_init(void)
{
	int ret;
	struct proc_dir_entry *dir;

	printk(KERN_INFO "SVFS ACPI Services Driver v%s\n", SVFS_ACPI_VERSION);

	ret = misc_register(&svfs_acpi_dev);
	if (ret) {
		printk(KERN_ERR "%s: unable to register \"misc\" device\n", __FUNCTION__);
		return ret;
	}

	dir = proc_create("driver/svfs_acpi", 0, NULL, &svfs_acpi_proc_ops);
	if (dir == NULL) {
		printk(KERN_ERR "%s: can't create /proc/driver/svfs_acpi\n", __FUNCTION__);
		misc_deregister(&svfs_acpi_dev);
		return -1;
	}
	return 0;
}

static void __exit
svfs_acpi_exit(void)
{
	/* not yet used */
}

module_init(svfs_acpi_init);
module_exit(svfs_acpi_exit);

MODULE_LICENSE("GPL");
