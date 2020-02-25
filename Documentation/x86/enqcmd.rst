Improved Device Interaction Overview

== Background ==

Typical hardware devices require a driver stack to translate application
buffers to hardware addresses, and a kernel-user transition to notify the
hardware of new work. What if both the translation and transition overhead
could be eliminated? This is what Shared Virtual Addressing (SVA) and ENQCMD
enabled hardware like DSA aims to achieve. Applications request their own
local-address-space portals to submit work and use a new instruction to post
work to the device.

== Address Space Tagging ==

A new MSR allows an application address space to be associated with what the
PCIe spec calls a PASID. This PASID tag is carried along with all requests
between applications and devices and allows devices to interact with the
process address space.

This MSR is managed with the XSAVE feature set as “supervisor state”. It needs
to be configured at the logical CPU level before each application thread
interacts with a device. It also needs to be cleared when new address spaces
are created (fork() and execve() time).  It _could_ be cleared once all portals have been unmapped, but managing that and forcing all threads to clear their
MSR is not worth the complexity.

Although optional, the MSR state is also cleared in the child thread at
clone(). This ensures that all threads have explicitly used the bind interfaces
and do not accidentally inherit a parent’s MSR value.

== PASID Management ==

The kernel must allocate a PASID on behalf of each process and program it
into the new MSR to communicate the process identity to platform hardware.

Threads that belong to the same process share the same MSR value. Each time
a new user address space is created (fork() and exec()), the PASID value is
reset and a new value must be allocated for the new address space.

Each thread is expected to bind to a PASID via one of the driver-level
XXX_bind_mm() calls before using ENQCND. A single process PASID can be used
simultaneously with multiple devices. New threads may not inherit the MSR
state from their parent and should always ensure they have made a
driver-provided ..._bind_mm() call.

== Relationships ==

 * Each process has many threads, but only one PASID
 * Devices have a limited number (~10’s to 1000’s) of hardware
   workqueues and each portal maps down to a single workqueue.
   The device driver manages allocating hardware workqueues.
 * A single mmap() maps a single hardware workqueue as a “portal”
 * For each device with which a process interacts, there must be
   one or more mmap()’d portals.
 * Many threads within a process can share a single portal to access
   a single device.
 * Multiple processes can separately mmap() the same portal, in
   which case they share one device hardware workqueue.
 * The single process-wide PASID is used by all threads to interact
   with all devices.  There is not, for instance, a PASID for each
   thread or each thread<->device pair.

FAQ:

What is SVA/SVM?

Shared Virtual Addressing (SVA) permits I/O hardware and the processor to
work in the same address space. In short, sharing the address space. Some
call it Shared Virtual Memory (SVM), but Linux community wanted to avoid
it with Posix Shared Memory and Secure Virtual Machines which were terms
already in circulation.


What is a PASID?

A Process Address Space ID (PASID) is a PCIe-defined TLP Prefix. A PASID
is a 20 bit number allocated and managed by the OS. PASID is included in
all transactions between the platform and the device.


How are shared work queues different?

Traditionally to allow user space applications interact with hardware, there
is a separate instance required per process. For e.g consider doorbells as
a mechanism of informing hardware about work to process. Each doorbell is
required to be spaced 4k (or page-size) apart for process isolation. This
requires hardware to provision that space and reserve in MMIO. This doesn’t
scale as the number of threads becomes quite large. The hardware also manages
the queue depth for SWQ, and consumers don’t need to track queue depth.
If there is no space to accept a command, the device will return an error
indicating retry. Also submitting a command to an MMIO address that can’t
accept ENQCMD will return retry in response.

Shared Work Queue (SWQ) allows hardware to provision just a single address
in the device. When used with ENQCMD to submit work, the device can
distinguish the process submitting the work since it will include the PASID
assigned to that process. This decreases the pressure of hardware requiring
to support hardware to scale to a large number of processes.


Is this the same as a userspace device driver?

Communicating with the device via the shared work queue is much simpler than
a full blown user space driver. The kernel driver does all the initialization
of the hardware. User space only needs to worry about submitting work and
processing completions.


Is this the same as SR-IOV?

Single Root I/O Virtualization (SR-IOV) focuses on providing independent
hardware interfaces for virtualizing hardware. Hence its required to be
almost fully functional interface to software supporting the traditional
BAR’s, space for interrupts via MSI-x, its own register layout. Creating
of this Virtual Functions (VFs) is assisted by the Physical Function (PF)
driver.

Scalable I/O Virtualization builds on the PASID concept to create device
instances for virtualization. SIOV requires host software to assist in
creating virtual devices, each virtual device is represented by a PASID.
This allows device hardware to optimize device resource creation and can
grow dynamically on demand. SR-IOV creation and management is very static
in nature. Consult references below for more details.


Why not just create a virtual function for each app?





Does this support virtualization?





Does memory need to be pinned?

When devices support SVA, along with platform hardware such as IOMMU
supporting such devices, there is no need to pin memory for DMA purposes.
Devices that support SVA also support other PCIe features that remove the
pinning requirement for memory.

Device TLB support - Device requests the IOMMU to lookup an address before
use via Address Translation Service (ATS) requests. If the mapping exists
but there is no page allocated by the OS, IOMMU hardware returns that no
mapping exists.

Device requests that virtual address to be mapped via Page Request
Interface (PRI). Once the OS has successfully completed  the mapping, it
returns the response back to the device. The device continues again to
request for a translation and continues.

IOMMU works with the OS in managing consistency of page-tables with the device.
When removing pages, it interacts with the device to remove any device-tlb
that might have been cached before removing the mappings from the OS.


References

VT-d:
https://01.org/blogs/ashokraj/2018/recent-enhancements-intel-virtualization-technology-directed-i/o-intel-vt-d

Intel Scalable IOV:
https://01.org/blogs/2019/assignable-interfaces-intel-scalable-i/o-virtualization-linux

ISE:
https://software.intel.com/sites/default/files/managed/c5/15/architecture-instruction-set-extensions-programming-reference.pdf

DSA:
https://software.intel.com/sites/default/files/341204-intel-data-streaming-accelerator-spec.pdf
