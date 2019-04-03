/* SPDX-License-Identifier: GPL-2.0 */
// Copyright (C) 2018 Hangzhou C-SKY Microsystems co.,ltd.

#include <linux/linkage.h>
#include <abi/entry.h>
#include <abi/pgtable-bits.h>
#include <asm/errno.h>
#include <asm/setup.h>
#include <asm/unistd.h>
#include <asm/asm-offsets.h>
#include <linux/threads.h>
#include <asm/setup.h>
#include <asm/page.h>
#include <asm/thread_info.h>

.macro tlbop_begin
	SAVE_ALL EPC_KEEP
#ifdef CONFIG_CPU_HAS_TLBI
	RD_MEH	a1
	tlbi.vaas a1
	sync.is
#else
	bgeni	a1, 31
	WR_MCIR	a1
	bgeni	a1, 25
	WR_MCIR	a1
#endif
.endm

.macro tlbop_end
	mov     a0, sp
	RD_MEH	a1
	psrset  ee, ie
	jbsr    do_page_fault
	jmpi    ret_from_exception
.endm

.text

ENTRY(csky_tlbinvalidl)
	tlbop_begin
	tlbop_end

ENTRY(csky_tlbinvalids)
	tlbop_begin
	tlbop_end

ENTRY(csky_tlbmodified)
	tlbop_begin
#ifndef CONFIG_CPU_HAS_LDSTEX
	jbsr csky_cmpxchg_fixup
#endif
	tlbop_end

ENTRY(csky_systemcall)
	SAVE_ALL EPC_INCREASE

	psrset  ee, ie

	lrw     r11, __NR_syscalls
	cmphs   syscallid, r11		/* Check nr of syscall */
	bt      ret_from_exception

	lrw     r13, sys_call_table
	ixw     r13, syscallid
	ldw     r11, (r13)
	cmpnei  r11, 0
	bf      ret_from_exception

	mov     r9, sp
	bmaski  r10, THREAD_SHIFT
	andn    r9, r10
	ldw     r8, (r9, TINFO_FLAGS)
	ANDI_R3	r8, (_TIF_SYSCALL_TRACE | _TIF_SYSCALL_TRACEPOINT | _TIF_SYSCALL_AUDIT)
	cmpnei	r8, 0
	bt      csky_syscall_trace
#if defined(__CSKYABIV2__)
	subi    sp, 8
	stw  	r5, (sp, 0x4)
	stw  	r4, (sp, 0x0)
	jsr     r11                      /* Do system call */
	addi 	sp, 8
#else
	jsr     r11
#endif
	stw     a0, (sp, LSAVE_A0)      /* Save return value */
	jmpi    ret_from_exception

csky_syscall_trace:
	mov	a0, sp                  /* sp = pt_regs pointer */
	jbsr	syscall_trace_enter
	/* Prepare args before do system call */
	ldw	a0, (sp, LSAVE_A0)
	ldw	a1, (sp, LSAVE_A1)
	ldw	a2, (sp, LSAVE_A2)
	ldw	a3, (sp, LSAVE_A3)
#if defined(__CSKYABIV2__)
	subi	sp, 8
	stw	r5, (sp, 0x4)
	stw	r4, (sp, 0x0)
#else
	ldw	r6, (sp, LSAVE_A4)
	ldw	r7, (sp, LSAVE_A5)
#endif
	jsr	r11                     /* Do system call */
#if defined(__CSKYABIV2__)
	addi	sp, 8
#endif
	stw	a0, (sp, LSAVE_A0)	/* Save return value */

	mov     a0, sp                  /* right now, sp --> pt_regs */
	jbsr    syscall_trace_exit
	br	ret_from_exception

ENTRY(ret_from_kernel_thread)
	jbsr	schedule_tail
	mov	a0, r8
	jsr	r9
	jbsr	ret_from_exception

ENTRY(ret_from_fork)
	jbsr	schedule_tail
	mov	r9, sp
	bmaski	r10, THREAD_SHIFT
	andn	r9, r10
	ldw	r8, (r9, TINFO_FLAGS)
	ANDI_R3	r8, (_TIF_SYSCALL_TRACE | _TIF_SYSCALL_TRACEPOINT | _TIF_SYSCALL_AUDIT)
	cmpnei	r8, 0
	bf	3f
	mov	a0, sp			/* sp = pt_regs pointer */
	jbsr	syscall_trace_exit
3:
	jbsr	ret_from_exception

ret_from_exception:
	ld	syscallid, (sp, LSAVE_PSR)
	btsti	syscallid, 31
	bt	1f

	/*
	 * Load address of current->thread_info, Then get address of task_struct
	 * Get task_needreshed in task_struct
	 */
	mov	r9, sp
	bmaski	r10, THREAD_SHIFT
	andn	r9, r10

resume_userspace:
	ldw	r8, (r9, TINFO_FLAGS)
	andi	r8, (_TIF_SIGPENDING | _TIF_NOTIFY_RESUME | _TIF_NEED_RESCHED)
	cmpnei	r8, 0
	bt	exit_work
1:  RESTORE_ALL

exit_work:
	btsti	r8, TIF_NEED_RESCHED
	bt	work_resched
	/* If thread_info->flag is empty, RESTORE_ALL */
	cmpnei	r8, 0
	bf	1b
	mov	a0, r8
	mov	a1, sp
	jbsr	do_notify_resume	/* do signals */
	br	resume_userspace

work_resched:
	lrw	syscallid, ret_from_exception
	mov	r15, syscallid		/* Return address in link */
	jmpi	schedule

ENTRY(sys_rt_sigreturn)
	jmpi	do_rt_sigreturn

ENTRY(csky_trap)
	SAVE_ALL EPC_KEEP
	psrset	ee
	mov	a0, sp                 /* Push Stack pointer arg */
	jbsr	trap_c                 /* Call C-level trap handler */
	jmpi	ret_from_exception

/*
 * Prototype from libc for abiv1:
 * register unsigned int __result asm("a0");
 * asm( "trap 3" :"=r"(__result)::);
 */
ENTRY(csky_get_tls)
	USPTOKSP

	/* increase epc for continue */
	mfcr	a0, epc
	INCTRAP	a0
	mtcr	a0, epc

	/* get current task thread_info with kernel 8K stack */
	bmaski	a0, THREAD_SHIFT
	not	a0
	subi	sp, 1
	and	a0, sp
	addi	sp, 1

	/* get tls */
	ldw	a0, (a0, TINFO_TP_VALUE)

	KSPTOUSP
	rte

ENTRY(csky_irq)
	SAVE_ALL EPC_KEEP
	psrset	ee

#ifdef CONFIG_PREEMPT
	mov	r9, sp			/* Get current stack  pointer */
	bmaski	r10, THREAD_SHIFT
	andn	r9, r10			/* Get thread_info */

	/*
	 * Get task_struct->stack.preempt_count for current,
	 * and increase 1.
	 */
	ldw	r8, (r9, TINFO_PREEMPT)
	addi	r8, 1
	stw	r8, (r9, TINFO_PREEMPT)
#endif

	mov	a0, sp
	jbsr	csky_do_IRQ

#ifdef CONFIG_PREEMPT
	subi	r8, 1
	stw	r8, (r9, TINFO_PREEMPT)
	cmpnei	r8, 0
	bt	2f
	ldw	r8, (r9, TINFO_FLAGS)
	btsti	r8, TIF_NEED_RESCHED
	bf	2f
1:
	jbsr	preempt_schedule_irq	/* irq en/disable is done inside */
	ldw	r7, (r9, TINFO_FLAGS)	/* get new tasks TI_FLAGS */
	btsti	r7, TIF_NEED_RESCHED
	bt	1b			/* go again */
#endif
2:
	jmpi	ret_from_exception

/*
 * a0 =  prev task_struct *
 * a1 =  next task_struct *
 * a0 =  return next
 */
ENTRY(__switch_to)
	lrw	a3, TASK_THREAD
	addu	a3, a0

	mfcr	a2, psr			/* Save PSR value */
	stw	a2, (a3, THREAD_SR)	/* Save PSR in task struct */
	bclri	a2, 6			/* Disable interrupts */
	mtcr	a2, psr

	SAVE_SWITCH_STACK

	stw	sp, (a3, THREAD_KSP)

	/* Set up next process to run */
	lrw	a3, TASK_THREAD
	addu	a3, a1

	ldw	sp, (a3, THREAD_KSP)	/* Set next kernel sp */

	ldw	a2, (a3, THREAD_SR)	/* Set next PSR */
	mtcr	a2, psr

#if  defined(__CSKYABIV2__)
	addi	r7, a1, TASK_THREAD_INFO
	ldw	tls, (r7, TINFO_TP_VALUE)
#endif

	RESTORE_SWITCH_STACK

	rts
ENDPROC(__switch_to)
