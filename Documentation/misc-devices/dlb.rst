.. SPDX-License-Identifier: (GPL-2.0-only OR BSD-2-Clause)

=======================================
Intel(R) Dynamic Load Balancer Overview
=======================================

:Author: Gage Eads

Contents
========

- Introduction
- Scheduling
- Query Entry
- Port
- Queue
- Credits
- Scheduling Domain
- Interrupts
- Power Management
- Virtualization
- User Interface

Introduction
============

The Intel(r) Dynamic Load Balancer (Intel(r) DLB) is a PCIe device that
provides load-balanced, prioritized scheduling of core-to-core communication.

The Intel DLB device consists of queues and arbiters that connect producer cores and
consumer cores. The device implements load-balanced queueing features including:
- Lock-free multi-producer/multi-consumer operation.
- Multiple priority levels for varying traffic types.
- 'Direct' traffic (i.e. multi-producer/single-consumer)
- Simple unordered load-balanced distribution.
- Atomic lock free load balancing across multiple consumers.
- Queue element reordering feature allowing ordered load-balanced distribution.

Intel DLB can be used in an event-driven programming model, such as DPDK's
Event Device Library[2]. Such frameworks are commonly used in packet processing
pipelines that benefit from the framework's multi-core scalability, dynamic
load-balancing, and variety of packet distribution and synchronization schemes.

Scheduling Types
================

Intel DLB supports four types of scheduling of 'events' (using DPDK
terminology), where an event can represent any type of data (e.g. a network
packet). The first, ``directed``, is multi-producer/single-consumer style
core-to-core communication. The remaining three are
multi-producer/multi-consumer, and support load-balancing across the consumers.

- ``Unordered``: events are load-balanced across consumers without any ordering
                 guarantees.

- ``Ordered``: events are load-balanced across consumers, and when the consumer
               re-injects each event into the device it is re-ordered into the
               original order. This scheduling type allows software to
               parallelize ordered event processing without the synchronization
               cost of re-ordering packets.

- ``Atomic``: events are load-balanced across consumers, with the guarantee that
              events from a particular 'flow' are only scheduled to a single
              consumer at a time (but can migrate over time). This allows, for
              example, packet processing applications to parallelize while
              avoiding locks on per-flow data and maintaining ordering within a
              flow.

Intel DLB provides hierarchical priority scheduling, with eight priority levels
within each. Each consumer selects up to eight queues to receive events from,
and assigns a priority to each of these 'connected' queues. To schedule an
event to a consumer, the device selects the highest priority non-empty queue
of the (up to) eight connected queues. Within that queue, the device selects
the highest priority event available (selecting a lower priority event for
starvation avoidance 1% of the time, by default).

Queue Entry
===========

Each event is contained in a queue entry (QE), the fundamental unit of
communication through the device, which consists of 8B of data and 8B of
metadata, as depicted below.

QE structure format
::
    data     :64
    opaque   :16
    qid      :8
    sched    :2
    priority :3
    msg_type :3
    lock_id  :16
    rsvd     :8
    cmd      :8

The ``data`` field can be any type that fits within 8B (pointer, integer,
etc.); DLB merely copies this field from producer to consumer. The ``opaque``
and ``msg_type`` fields behave the same way.

``qid`` is set by the producer to specify to which DLB queue it wishes to
enqueue this QE. The ID spaces for load-balanced and directed queues are both
zero-based; the ``sched`` field is used to distinguish whether the queue is
load-balanced or directed.

``sched`` controls the scheduling type: atomic, unordered, ordered, or
directed. The first three scheduling types are only valid for load-balanced
queues, and the directed scheduling type is only valid for directed queues.

``priority`` is the priority with which this QE should be scheduled.

``lock_id``, used only by atomic scheduling, identifies the atomic flow to
which the QE belongs. When sending a directed event, ``lock_id`` is simply
copied like the ``data``, ``opaque``, and ``msg_type`` fields.

``cmd`` specifies the operation, such as:
- Enqueue a new QE
- Forward a QE that was dequeued
- Complete/terminate a QE that was dequeued
- Return one or more consumer queue tokens.
- Arm the port's consumer queue interrupt.

Port
====

A core's interface to the DLB is called a "port," and consists of an MMIO region
through which the core enqueues a queue entry, and an in-memory queue (the
"consumer queue") to which the device schedules QEs. A core enqueues a QE to a
device queue, then the device schedules the event to a port. Software specifies
the connection of queues and ports; i.e. for each queue, which port the device
is allowed to schedule its events to. The device uses a credit scheme to
prevent overflow of the on-device queue storage.

Applications can interface directly with the device by mapping the port's
memory and MMIO regions into the application's address space for enqueue and
dequeue operations, but call into the kernel driver for configuration
operations. An application can also be polling- or interrupt-driven; DLB
supports both modes of operation.

Queue
=====

The device contains 128 load-balanced (i.e. capable of atomic, ordered, and
unordered scheduling) queues and 128 directed queues.

Credits
=======

The Intel DLB uses a credit scheme to prevent overflow of the on-device queue
storage, with separate credits for load-balanced and directed queues. A port
spends one credit when it enqueues a QE, and one credit is replenished when a
QE is scheduled to a consumer queue. Credits are divided into pools, and each
port belongs to a directed credit pool and a load-balanced credit pool. When a
port's credit allocation reaches a (configurable) low watermark, the device
replenishes them from the pool (if available).

Scheduling Domain
=================

Device resources -- i.e. ports, queues, credits -- are contained within a
scheduling domain. Scheduling domains are isolated from one another; a port can
only enqueue to and dequeue from queues within its scheduling domain. The
kernel driver creates a device file for each scheduling domain created by an
application, so access to a scheduling domain's resources can be restricted via
file permissions.

Consumer Queue Interrupts
=========================

Each port has its own interrupt which fires, if armed, when the consumer queue
depth becomes non-zero. An application can arm an interrupt by enqueueing a
special 'interrupt arm' command to the device through the port's MMIO window.

Power Management
================

The kernel driver keeps the device in D3Hot when not in use. The driver
transitions the device to D0 when the first device file is opened or a virtual
function is created, and keeps it there until there are no open device files,
memory mappings, or virtual functions.

Virtualization
==============

The DLB supports SR-IOV virtualization, and can flexible divide its resources
among the PF and its VFs. VFs do not configure the device directly; they use a
hardware mailbox to proxy configuration requests to the PF driver. VF and PF
communication begins with a registration message that establishes the mailbox
interface version.

Each VF has 32 interrupts, 1 for PF->VF mailbox messages and the remainder for
CQ interrupts. If a VF user (e.g. a guest OS) needs more CQ interrupts, they
have to use more than one VF.

To support this case, the driver introduces the notion of primary and auxiliary
VFs. A VF is either considered primary or auxiliary:
- Primary: the VF is used as a regular DLB device. The primary VF has 0+
           auxiliary VFs supporting it.
- Auxiliary: the VF doesn't have any resources of its own, and serves only to
             provide the primary VF with MSI vectors for its CQ interrupts.

Each VF has an aux_vf_ids file in its sysfs directory, which is a R/W file that
controls the primary VF’s auxiliaries. When a VF is made auxiliary to another,
its resources are relinquished to the PF device.

When the VF driver registers its device with the PF driver, the PF driver tells
the VF driver whether its device is auxiliary or primary, and if so then the ID
of its primary VF. If it is auxiliary, the VF device will “claim” up to 31 of
the primary VF’s CQs, such that they use the auxiliary VF’s MSI vectors.

When a primary VF has one or more auxiliary VFs, the entire VF group must be
assigned to the same virtual machine. The PF driver will not allow the primary
VF to configure its resources until all its auxiliary VFs have been registered
by the guest OS’s driver.

User Interface
==============

The dlb driver uses ioctls as its primary interface. It provides two types of
device files: the dlb device file and the scheduling domain device file.

The two types support different ioctl interfaces; the dlb device file is used
for device-wide operations (including scheduling domain creation), and the
scheduling domain device file supports operations on the scheduling domain's
resources such as port and queue configuration. The scheduling domain device
file is also exports an mmap interface for port memory and its MMIO window.
