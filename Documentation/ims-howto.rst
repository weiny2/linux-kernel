.. SPDX-License-Identifier: GPL-2.0
.. include:: <isonum.txt>

==========================
The IMS Driver Guide HOWTO
==========================

:Authors: Megha Dey

:Copyright: 2020 Intel Corporation

About this guide
================

This guide describes the basics of Interrupt Message Store (IMS), the
need to introduce a new interrupt mechanism, implementation details of
IMS in the kernel, driver changes required to support IMS and the general
misconceptions and FAQs associated with IMS.

What is IMS?
============

Intel has introduced the Scalable I/O virtualization (SIOV)[1] which
provides a scalable and lightweight approach to hardware assisted I/O
virtualization by overcoming many of the shortcomings of SR-IOV.

SIOV shares I/O devices at a much finer granularity, the minimal sharable
resource being the 'Assignable Device Interface' or ADI. Each ADI can
support multiple interrupt messages and thus, we need a matching scalable
interrupt mechanism to process these ADI interrupts. Interrupt Message
Store or IMS is a new interrupt mechanism to meet such a demand.

Why use IMS?
============

Until now, the maximum number of interrupts a device could support was 2048
(using MSI-X). With IMS, there is no such restriction. A device can report
support for SIOV(and hence IMS) to the kernel through the host device
driver. Alternatively, if the kernel needs a generic way to discover these
capabilities without host driver dependency, the PCIE Designated Vendor
specific Extended capability(DVSEC) can be used. ([1]Section 3.7)

IMS is device-specific which means that the programming of the interrupt
messages (address/data pairs) is done in some device specific way, and not
by using a standard like PCI. Also, by using IMS, the device is free to
choose where it wants to store the interrupt messages. This makes IMS
highly flexible. Some devices may organise IMS as a table in device memory
(like MSI-X) which can be accessed through one or more memory mapped system
pages, some device implementations may organise it in a distributed/
replicated fashion at each of the “engines” in the device (with future
multi-tile devices) while context based devices (GPUs for instance),
can have it stored/located in memory (as part of supervisory state of a
command/context), that the hosting function can fetch and cache on demand
at the device. Since the number of contexts cannot be determined at boot
time, there cannot be a standard enumeration of the IMS size during boot.
In any approach, devices may implement IMS as either one unified storage
structure or de-centralized per ADI storage structures.

Even though the IMS storage organisation is device-specific, IMS entries
store and generate interrupts using the same interrupt message address and
data format as the PCI Express MSI-X table entries, a DWORD size data
payload and a 64-bit address. Interrupt messages are expected to be
programmed only by the host driver. All the IMS interrupt messages are
stored in the remappable format. Hence, if a driver enables IMS, interrupt
remapping is also enabled by default.

A device can support both MSI-X and IMS entries simultaneously, each being
used for a different purpose. E.g., MSI-X can be used to report device level
errors while IMS for software constructed devices created for native or
guest use.

Implementation of IMS in the kernel
===================================

The Linux kernel today already provides a generic mechanism to support
non-PCI compliant MSI interrupts for platform devices (platform-msi.c).
To support IMS interrupts, we create a new IMS IRQ domain and extend the
existing infrastructure. Dynamic allocation of IMS vectors is a requirement
for devices which support Scalable I/O Virtualization. A driver can allocate
and free vectors not just once during probe (as was the case with MSI/MSI-X)
but also in the post probe phase where actual demand is available. Thus, a
new API, platform_msi_domain_alloc_irqs_group is introduced which drivers
using IMS would be able to call multiple times. The vectors allocated each
time this API is called are associated with a group ID. To free the vectors
associated with a particular group, the platform_msi_domain_free_irqs_group
API can be called. The existing drivers using platform-msi infrastructure
will continue to use the existing alloc (platform_msi_domain_alloc_irqs)
and free (platform_msi_domain_free_irqs) APIs and are assigned a default
group ID of 0.

Thus, platform-msi.c provides the generic methods which can be used by any
non-pci MSI interrupt type while the newly created ims-msi.c provides IMS
specific callbacks that can be used by drivers capable of generating IMS
interrupts. Intel has introduced data streaming accelerator (DSA)[2] device
which supports SIOV and thus supports IMS. Currently, only Intel's Data
Accelerator (idxd) driver is a consumer of this feature.

FAQs and general misconceptions:
================================

** There were some concerns raised by Thomas Gleixner and Marc Zyngier
during Linux plumbers conference 2019:

1. Enumeration of IMS needs to be done by PCI core code and not by
   individual device drivers:

   Currently, if the kernel needs a generic way to discover IMS capability
   without host driver dependency, the PCIE Designated Vendor specific

   However, we cannot have a standard way of enumerating the IMS size
   because for context based devices, the interrupt message is part of
   the context itself which is managed entirely by the driver. Since
   context creation is done on demand, there is no way to tell during boot
   time, the maximum number of contexts (and hence the number of interrupt
   messages)that the device can support.

   Also, this seems redundant (given only the driver will use this
   information). Hence, we thought it may suffice to enumerate it as part
   of driver callback interfaces. In the current linux code, even with
   MSI-X, the size reported by MSI-X capability is used only to cross check
   if the driver is asking more than that or not (and if so, fail the call).

   Although, if you believe it would be useful, we can add the IMS size
   enumeration to the SIOV DVSEC capability.

   Perhaps there is a misunderstanding on what IMS serves. IMS is not a
   system-wide interrupt solution which serves all devices; it is a
   self-serving device level interrupt mechanism (other than using system
   vector resources). Since both producer and consumer of IMS belong to
   the same device driver, there wouldn't be any ordering problem. Whereas,
   if IMS service is provided by one driver which serves multiple drivers,
   there would be ordering problems to solve.

Some other commonly asked questions about IMS are as follows:

1. Does all SIOV devices support MSI-X (even if they have IMS)?

   Yes, all SIOV hosting functions are expected to have MSI-X capability
   (irrespective of whether it supports IMS or not). This is done for
   compatibility reasons, because a SIOV hosting function can be used
   without enabling any SIOV capabilities as a standard PCIe PF.

2. Why is Intel designing a new interrupt mechanism rather than extending
   MSI-X to address its limitations? Isn't 2048 device interrupts enough?

   MSI-X has a rigid definition of one-table and on-device storage and does
   not provide the full flexibility required for future multi-tile
   accelerator designs.
   IMS was envisioned to be used with large number of ADIs in devices where
   each will need unique interrupt resources. For example, a DSA shared
   work queue can support large number of clients where each client can
   have its own interrupt. In future, with user interrupts, we expect the
   demand for messages to increase further.

3. Will there be devices which only support IMS in the future?

   No. All Scalable IOV devices will support MSI-X. But the number of MSI-X
   table entries may be limited compared to number of IMS entries. Device
   designs can restrict the number of interrupt messages supported with
   MSI-X (e.g., support only what is required for the base PF function
   without SIOV), and offer the interrupt message scalability only through
   IMS. For e.g., DSA supports only 9 messages with MSI-X and 2K messages
   with IMS.

Device Driver Changes:
=====================

1. platform_msi_domain_alloc_irqs_group (struct device *dev, unsigned int
   nvec, const struct platform_msi_ops *platform_ops, int *group_id)
   to allocate IMS interrupts, where:

   dev: The device for which to allocate interrupts
   nvec: The number of interrupts to allocate
   platform_ops: Callbacks for platform MSI ops (to be provided by driver)
   group_id: returned by the call, to be used to free IRQs of a certain type

   eg: static struct platform_msi_ops ims_ops  = {
        .irq_mask               = ims_irq_mask,
        .irq_unmask             = ims_irq_unmask,
        .write_msg              = ims_write_msg,
        };

        int group;
        platform_msi_domain_alloc_irqs_group (dev, nvec, platform_ops, &group)

   where, struct platform_msi_ops:
   irq_mask:   mask an interrupt source
   irq_unmask: unmask an interrupt source
   irq_write_msi_msg: write message content

   This API can be called multiple times. Every time a new group will be
   associated with the allocated vectors. Group ID starts from 0.

2. platform_msi_domain_free_irqs_group(struct device *dev, int group) to
   free IMS interrupts from a particular group

3. To traverse the msi_descs associated with a group:
        struct device *device;
        struct msi_desc *desc;
        struct platform_msi_group_entry *platform_msi_group;
        int group;

        for_each_platform_msi_entry_in_group(desc, platform_msi_group, group, dev) {
        }

References:
===========

[1]https://software.intel.com/en-us/download/intel-scalable-io-virtualization-technical-specification
[2]https://software.intel.com/en-us/download/intel-data-streaming-accelerator-preliminary-architecture-specification
