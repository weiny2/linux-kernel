.. SPDX-License-Identifier: GPL-2.0

============================================================
Hardware-Feedback Interface for scheduling on Intel Hardware
============================================================

Overview
--------

Intel has described the Hardware Feedback Interface (HFI) in the Intel
Architecture Set Extensions and Future Features Programming Reference [1]_.
This document presents a summary of such document and how it is used in Linux
to define the CPU priority when scheduling tasks.

The HFI gives the operating system a performance and energy efficiency rating
for each logical processor in the system. Following a similar approach to that
of asymmetric packing of tasks, Linux can dynamically use this information to
give higher priority to CPUs with higher performance. Linux can thus react to
dynamic changes in operating conditions by updating the priority, which may
migrate tasks from lower CPUs to higher priority CPUs.

HFI Goals
---------

A hybrid processor contains one or more Core CPUs, and one or more Atom CPUs.

HFI's first goal is to assure optimal burst performance for single-thread-
dominant workloads on hybrid processors. Linux can use HFI to identify the
highest performance CPUs, and place performance-demanding threads on those
CPUS.

HFI's also allows Linux to identify the most power efficient CPUs. They may be
most optimal for running non-performance-critical background tasks, or when the
entire system is in a low power mode.

Finally, if the system becomes constrained in power or temperature, the Core
CPUs may be throttled such that the Atoms may be faster. HFI will detect and
reflect this performance inversion, allowing Linux to react and prefer the new
set of highest performing CPUs.

In the current implementation, Linux takes advantage of HFI's performance
ranking of CPUs.  This allows it to optimally handle single-threaded
performance workloads, and also detect the performance inversion case. Linux
currently does not have a mechanism to favor the most efficient CPUs, when the
most efficient CPUs are different from the most performant CPUs.

The Hardware Feedback Interface
-------------------------------

The Hardware Feedback Algorithm
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

The Hardware Feedback Interface provides to the operating system two ratings for
each logical processor in the system: performance and energy efficiency. Each
capability is given as a unit-less quantity in the range [0-255]; where higher
values indicate higher performance and energy efficiency. It is important to
note that these capabilities may change at runtime  as a result of changes in
the operating conditions of the system and a result of the action of external
agents.

A hardware-specific algorithm continuously computes the performance and energy
efficiency of each logical processor given the system workload as well as power
and thermal limits. For instance, if the system has various types of CPUs, it
may use the instructions-per-clock cycle (IPC) ratio between those CPUs (i.e.,
CPUs with higher IPC will have a higher performance capability). If the system
is running on a restrictive power limit (e.g., using the RAPL interfaces), CPUs
with higher power consumption will be given a lower performance capability. If
the package reaches a thermal limit, CPUs less able to dissipate heat will
receive a lower performance rating. It follows that the operating system can
use this information to decide onto which CPUs to run tasks.

The structure of the Hardware Feedback Interface Table
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

Hardware provides feedback to the operating system using a table in memory. The
memory region is provided in advance by the operating system. Please see [1]_
for full details.

The format of the table is as follows:

+------------------+-----------------+-----------------------------------+
| Byte offset      | Size (bytes)    |          Description              |
+==================+=================+===================================+
|         0        |       16        | Global header                     |
+------------------+-----------------+-----------------------------------+
|        16        |        8        | Logical processor 0 capabilities  |
+------------------+-----------------+-----------------------------------+
|        24        |        8        | Logical processor 1 capabilities  |
+------------------+-----------------+-----------------------------------+
|       ...        |      ...        | ...                               |
+------------------+-----------------+-----------------------------------+
|     16 + n*8     |        8        | logical processor n capabilities  |
+------------------+-----------------+-----------------------------------+

The global header has the following format:

+--------+--------+-------------+----------------------------------------+
| Byte   | Size   |   Field     |             Description                |
| offset | (bytes)|             |                                        |
+========+========+=============+========================================+
|   0    |   8    | Time stamp  | Time stamp in clock crystal units of   |
|        |        |             | the last time this table was updated   |
|        |        |             | hardware.                              |
+--------+--------+-------------+----------------------------------------+
|   8    |   1    | Performance | If set to 1, indicates the performance |
|        |        | capability  | capability of one or more logical      |
|        |        | changed     | processor changed.                     |
+--------+--------+-------------+----------------------------------------+
|   9    |   1    | Energy      | If set to 1, indicates the energy      |
|        |        | efficiency  | efficiency capability of one or more   |
|        |        | capability  | logical processors changed             |
|        |        | changed     |                                        |
+--------+--------+-------------+----------------------------------------+
|   10   |   6    | Reserved    |                                        |
+--------+--------+-------------+----------------------------------------+

The capability changed bits in the header allow software to easily detect table
updates.

Each logical processor has an entry in the HFI table as follows:

+--------+--------+-------------+----------------------------------------+
| Byte   | Size   |    Field    |             Description                |
| offset | (bytes)|             |                                        |
+========+========+=============+========================================+
|   0    |   1    | Performance | Performance capability is an 8-bit     |
|        |        | capability  | value specifying the relative          |
|        |        |             | performance level of a logical         |
|        |        |             | processor. Higher values indicate      |
|        |        |             | higher performance.                    |
+--------+--------+-------------+----------------------------------------+
|   1    |   1    | Energy      | Energy efficiency capability is an 8-  |
|        |        | efficiency  | bit value specifying the relative      |
|        |        | capability  | energy efficiency level of a logical   |
|        |        |             | processor.                             |
+--------+--------+-------------+----------------------------------------+
|   2    |   6    | Reserved    | Initialized to 0 by the operating      |
|        |        |             | system                                 |
+--------+--------+-------------+----------------------------------------+

Hardware Feedback Interface Table
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

The operating system can opt to either poll the table periodically to look for
updates or receive an interrupt whenever the hardware updates the table.

Linux uses the interrupt mechanism. The HFI interrupt reuses the facilities
implemented to handle thermal management events. Further details can be found
in the Intel 64 and IA-32 Architectures Software Developer's Manual Vol. 3
Section 14.8 [2]_. New package-level interrupt and status bits are added to the
IA32_PACKAGE_THERM_INTERRUPT and IA32_PACKAGE_THERM_STATUS registers,
respectively. An interrupt happens when hardware has updated the HFI table and
is ready for the operating system to consume. CPUs receive such interrupt via
the thermal entry in the Local APIC's LVT.

When receiving the interrupt, Linux updates per-CPU data structures that keep
track of the performance rating and time stamp.

Linux Hardware Feedback Interface Table Use
-------------------------------------------

Linux already has the concept of CPUs with heterogeneous performance through
the ACPI _CPC object method and the HWP_CAPABILITIES. The _CPC object method
provides a field to indicate the highest performance of each CPU in a unit-
less scale. The value in the HWP_CAPABILITIES MSR represents the maximum
non-guaranteed performance of a CPU without any power or thermal constraints.

Since Linux v4.9, this information is used to support Intel Turbo Boost Max
Turbo 3.0. At boot, the intel_pstate driver inspects the contents of the
Highest Performance register of each CPU from the _CPC object method. This
information is used to assign the priority of each CPU when using the
scheduler's asymmetric packing of tasks. In this manner, CPUs with higher
performance are given higher priority by the scheduler when scheduling tasks.

It is important to note that using the Highest Performance values of the ACPI
_CPC object method assumes that the heterogeneous performance capabilities of
CPUs are static. When using the data from the HFI table this is no longer the
case -- the performance ratings of CPUs may change during runtime.

The Linux scheduler consults the values returned by the architecture-specific
implementation of arch_asym_cpu_priority() to decide where place or migrate
tasks. The x86 implementation returns priorities derived from the _CPC ACPI
method. This is the mechanism used to support Intel Turbo Boost Max Technology
3.0 (ITMT). Unlike ITMT, when using the HFI table, the CPU priority becomes
dynamic.

With asymmetric packing of tasks, CPUs with higher priority are preferred when
placing or migrating tasks. If the priority of a given CPU dynamically
decreases, tasks may be migrated away from it towards CPUs with higher
priority.

References
----------

.. [1] https://software.intel.com/sites/default/files/managed/c5/15/architecture-instruction-set-extensions-programming-reference.pdf
.. [2] https://software.intel.com/en-us/articles/intel-sdm
.. [3] https://sharepoint.gar.ith.intel.com/sites/Lakefield/lkfhas/Shared%20Documents/HAS/Release/HAS%201.0%20Release/LKF-C%20Hetero%20HAS%201p0.pdf
