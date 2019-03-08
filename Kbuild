# SPDX-License-Identifier: GPL-2.0
#
# Kbuild for top-level directory of the kernel
<<<<<<< HEAD
# This file takes care of the following:
# 1) Generate bounds.h
# 2) Generate timeconst.h
# 3) Generate asm-offsets.h (may need bounds.h and timeconst.h)
# 4) Check for missing system calls
# 5) check atomics headers are up-to-date
# 6) Generate constants.py (may need bounds.h)
=======
>>>>>>> linux-next/akpm-base

#####
# Generate bounds.h

bounds-file := include/generated/bounds.h

always  := $(bounds-file)
targets := kernel/bounds.s

$(bounds-file): kernel/bounds.s FORCE
	$(call filechk,offsets,__LINUX_BOUNDS_H__)

#####
# Generate timeconst.h

timeconst-file := include/generated/timeconst.h

targets += $(timeconst-file)

filechk_gentimeconst = echo $(CONFIG_HZ) | bc -q $<

$(timeconst-file): kernel/time/timeconst.bc FORCE
	$(call filechk,gentimeconst)

#####
# Generate asm-offsets.h

offsets-file := include/generated/asm-offsets.h

always  += $(offsets-file)
targets += arch/$(SRCARCH)/kernel/asm-offsets.s

arch/$(SRCARCH)/kernel/asm-offsets.s: $(timeconst-file) $(bounds-file)

$(offsets-file): arch/$(SRCARCH)/kernel/asm-offsets.s FORCE
	$(call filechk,offsets,__ASM_OFFSETS_H__)

#####
# Check for missing system calls

always += missing-syscalls
targets += missing-syscalls

quiet_cmd_syscalls = CALL    $<
      cmd_syscalls = $(CONFIG_SHELL) $< $(CC) $(c_flags) $(missing_syscalls_flags)

missing-syscalls: scripts/checksyscalls.sh $(offsets-file) FORCE
	$(call cmd,syscalls)

#####
<<<<<<< HEAD
# 5) Check atomic headers are up-to-date
#

always += old-atomics
targets += old-atomics

quiet_cmd_atomics = CALL    $<
      cmd_atomics = $(CONFIG_SHELL) $<

old-atomics: scripts/atomic/check-atomics.sh FORCE
	$(call cmd,atomics)

#####
# 6) Generate constants for Python GDB integration
=======
# Check atomic headers are up-to-date
>>>>>>> linux-next/akpm-base
#

always += old-atomics
targets += old-atomics

quiet_cmd_atomics = CALL    $<
      cmd_atomics = $(CONFIG_SHELL) $<

old-atomics: scripts/atomic/check-atomics.sh FORCE
	$(call cmd,atomics)

# Keep these three files during make clean
no-clean-files := $(bounds-file) $(offsets-file) $(timeconst-file)
