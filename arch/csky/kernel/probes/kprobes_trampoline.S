/* SPDX-License-Identifier: GPL-2.0+ */

#include <linux/linkage.h>

#include <abi/entry.h>

ENTRY(kretprobe_trampoline)
	SAVE_ALL_KRETPROBE 0

	mov	a0, sp /* pt_regs */

	jbsr	trampoline_probe_handler

	/* use the result as the return-address */
	mov	lr, a0

	RESTORE_ALL_KRETPROBE
	rts
ENDPROC(kretprobe_trampoline)
