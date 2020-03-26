/*
 * SVFS ACPI Driver
 *
 * Copyright (C) 2010 Intel Corporation
 *
 */

#ifndef _SVFS_ACPI_H
#define _SVFS_ACPI_H

#include <linux/types.h>

#ifndef __KERNEL__
#include <sys/ioctl.h>
typedef void * acpi_handle;
#endif

typedef struct {
	acpi_handle	parent_obj;	// ACPI handle to parent PCI bus' object 
	unsigned int device;
	unsigned int function;
	unsigned int pin;
	unsigned int gsi;
	unsigned int busno;
	unsigned int segno;
	acpi_handle pirq_obj;	// ACPI handle to device's LNK object
	unsigned int pirq;
	unsigned int pirq_irq;
} svfs_irq_src_t;

#define ACPI_PCI_LINK_HID	"PNP0C0F"
#define MAX_PIRQ	8

typedef struct {
	int	mode;
} picArg_t;

typedef struct {
	int	total_bytes;
	int num_entries;
} walkSize_t;


typedef struct rdioapic {
	u32 full_abar;
	u8 index;
	u32 value;
} rdioapic_t;

#define SVFS_ACPI_IOCTL		'z'
#define SVFS_ACPI_PREP		_IO(SVFS_ACPI_IOCTL, 0)
#define SVFS_ACPI_PIC		_IO(SVFS_ACPI_IOCTL, 1)
#define SVFS_ACPI_WALK_LNK	_IO(SVFS_ACPI_IOCTL, 2)
#define SVFS_ACPI_WALK_PRT	_IO(SVFS_ACPI_IOCTL, 3)
#define SVFS_ACPI_WALK_SIZE	_IO(SVFS_ACPI_IOCTL, 4)
#define SVFS_ACPI_WALK_DATA	_IO(SVFS_ACPI_IOCTL, 5)
#define SVFS_ACPI_READ_IOAPIC	_IOWR(SVFS_ACPI_IOCTL, 6, rdioapic_t )
#define SVFS_ACPI_GET_RSDP	_IO(SVFS_ACPI_IOCTL, 7)

#endif
