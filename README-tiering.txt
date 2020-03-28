0. Introduction
===============

This document describes the software setup steps and usage hints that can be
helpful in setting up a system to use tiered memory for evaluation.

The document will go over:
1. Any kernel config options required to enable the tiered memory features
2. Any additional userspace tooling required, and related instructions
3. Any post-boot setup for configurable or tunable knobs

Note:
Any instructions/settings described here may be tailored to the branch this
is under. Setup steps may change from release to release, and for each
release branch, the setup document accompanying that branch should be
consulted.

1. Kernel build and configuration
=================================

a. The recommended starting point is a distro-default kernel config. We
   use and recommend using a recent Fedora config for a starting point.

b. Ensure the following:
   CONFIG_DEV_DAX_KMEM=m
   CONFIG_MEMORY_HOTPLUG_DEFAULT_ONLINE=n
   NUMA_BALANCING=y


2. Tooling setup
================

a. Install 'ndctl' and 'daxctl' from your distro, or from upstream:
   https://github.com/pmem/ndctl
   This may especially be required if the distro version of daxctl
   is not new enough to support the daxctl reconfigure-device command[1]

   [1]: https://pmem.io/ndctl/daxctl-reconfigure-device.html

b. Assuming that persistent memory devices are the next demotion tier
   for system memory, perform the following steps to allow a pmem device
   to be hot-plugged system RAM:

   First, convert 'fsdax' namespace(s) to 'devdax':
     ndctl create-namespace -fe namespaceX.Y -m devdax

c. Reconfigure 'daxctl' devices to system-ram using the kmem facility:
     daxctl reconfigure-device -m system-ram daxX.Y.

   The JSON emitted at this step contains the 'target_node' for this
   hotplugged memory. This is the memory-only NUMA node where this
   memory appears, and can be used explicitly with normal libnuma/numactl
   techniques.

d. Ensure the newly created NUMA nodes for the hotplugged memory are in
   ZONE_MOVABLE. The JSON from daxctl in the above step should indicate
   this with a 'movable: true' attribute. Based on the distribution, there
   may be udev rules that interfere with memory onlining. They may race to
   online memory into ZONE_NORMAL rather than movable. If this is the case,
   find and disable any such udev rules.

3. Post boot setup
==================

a. Setup migration targets
    # echo 2 > /sys/devices/system/node/node0/migration_path
    # echo 3 > /sys/devices/system/node/node1/migration_path
   etc.
   The number of nodes this will need to be performed for depends
   on CPU nodes in the system, and CPU-less nodes added in the previous step.
   In this example, The hotplugged pmem physically attached to node0 is node2,
   so the demotion path is node0 -> node2

b. Enable 'autonuma'
    # echo 2 > /proc/sys/kernel/numa_balancing
    # echo 30 > /proc/sys/kernel/numa_balancing_rate_limit_mbps

c. Enable swapcache promotion
   This is only valid when swap is turned on. When the option is enabled, it
   will migrate the hot swapcache page to fast DRAM node when possible.
    # sysctl -w vm.enable_swapcache_promotion=1

   The promoted page number could be checked in /proc/vmstat:
      hmem_swapcache_promote_src
      hmem_swapcache_promote_dst

d. Enable page cache promotion
   Support cache page promotion in shmem/generic file reads and writes. The
   demotion is on the page reclaim path. Provide user interfaces to
   control the rate limits of promotion and demotion. By default, promotion
   is disabled and demotion is enabled.

   For performance experiments, the recommended settings are:
   To enable promotion, set a non-zero ratelimit. Using '-1' disables any
   ratelimiting, and also enables promotion.
    # echo -1 > /proc/sys/vm/promotion_ratelimit_mbytes_per_sec
    # echo -1 > /proc/sys/vm/demotion_ratelimit_mbytes_per_sec

   The number of promoted pages can be checked by the following counters in
   /proc/vmstat or /sys/devices/system/node/node[n]/vmstat:
      hmem_reclaim_promote_src
      hmem_reclaim_promote_dst
      nr_promoted

   The number of pages demoted can be checked by the following counters:
      hmem_reclaim_demote_src
      hmem_reclaim_demote_dst

   The page number of failure in promotion could be checked by the
   following counters:
      nr_promote_fail
      nr_promote_isolate_fail

   The number of pages that have been limited by the ratelimit can be checked
   by the following counter:
      nr_promote_ratelimit

