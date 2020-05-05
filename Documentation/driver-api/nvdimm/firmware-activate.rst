.. SPDX-License-Identifier: GPL-2.0

==================================
NVDIMM Runtime Firmware Activation
==================================

Some persistent memory devices run a firmware locally on the device /
"DIMM" to perform tasks like media management, capacity provisioning,
and health monitoring. The process of updating that firmware typically
involves a reboot because it has implications for in-flight memory
transactions. However, reboots are disruptive and at least the Intel
persistent memory platform implementation, described by the Intel ACPI
DSM specification [1], has added support for activating firmware at
runtime.

A native sysfs interface is implemented in libnvdimm to allow platform
to advertise and control their local runtime firmware activation
capability.

The libnvdimm bus object, ndbusX, implements an ndbusX/firmware_activate
attribute that shows the state of the firmware activation as one of 'idle',
'armed', 'overflow', and 'busy'.

- idle:
  No devices are set / armed to activate firmware

- armed:
  At least one device is armed

- busy:
  In the busy state armed devices are in the process of transitioning
  back to idle and completing an activation cycle.

- overflow:
  If the platform has a concept of incremental work needed to perform
  the activation it could be the case that too many DIMMs are armed for
  activation. In that scenario the potential for firmware activation to
  timeout is indicated by the 'overflow' state.

The libnvdimm memory-device / DIMM object, nmemX, implements
nmemX/firmware_activate and nmemX/firmware_activate_result attributes to
communicate the per-device firmware activation state. Similar to the
ndbusX/firmware_activate attribute, the nmemX/firmware_activate
attribute indicates 'idle', 'armed', or 'busy'. The state transitions
from 'armed' to 'idle' when the system is prepared to activate firmware,
firmware staged + state set to armed, and a system sleep state
transition is triggered. After that activation event the
nmemX/firmware_activate_result attribute reflects the state of the last
activation as one of:

- none:
  No runtime activation triggered since the last time the device was reset

- success:
  The last runtime activation completed successfully.

- fail:
  The last runtime activation failed for device-specific reasons.

- not_staged:
  The last runtime activation failed due to a sequencing error of the
  firmware image not being staged.

- need_reset:
  Runtime firmware activation failed, but the firmware can still be
  activated via the legacy method of power-cycling the system.

[1]: https://docs.pmem.io/persistent-memory/
