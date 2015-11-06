/*
 * Dynamic function tracer architecture backend.
 *
 * Copyright IBM Corp. 2009,2014
 *
 *   Author(s): Heiko Carstens <heiko.carstens@de.ibm.com>,
 *		Martin Schwidefsky <schwidefsky@de.ibm.com>
 */

#include <linux/moduleloader.h>
#include <linux/hardirq.h>
#include <linux/uaccess.h>
#include <linux/ftrace.h>
#include <linux/kernel.h>
#include <linux/types.h>
#include <linux/kprobes.h>
#include <trace/syscall.h>
#include <asm/asm-offsets.h>
#include <asm/cacheflush.h>
#include "entry.h"

/*
 * The mcount code looks like this:
 *	stg	%r14,8(%r15)		# offset 0
 *	larl	%r1,<&counter>		# offset 6
 *	brasl	%r14,_mcount		# offset 12
 *	lg	%r14,8(%r15)		# offset 18
 * Total length is 24 bytes. Only the first instruction will be patched
 * by ftrace_make_call / ftrace_make_nop.
 * The enabled ftrace code block looks like this:
 * >	brasl	%r0,ftrace_caller	# offset 0
 *	larl	%r1,<&counter>		# offset 6
 *	brasl	%r14,_mcount		# offset 12
 *	lg	%r14,8(%r15)		# offset 18
 * The ftrace function gets called with a non-standard C function call ABI
 * where r0 contains the return address. It is also expected that the called
 * function only clobbers r0 and r1, but restores r2-r15.
 * For module code we can't directly jump to ftrace caller, but need a
 * trampoline (ftrace_plt), which clobbers also r1.
 * The return point of the ftrace function has offset 24, so execution
 * continues behind the mcount block.
 * The disabled ftrace code block looks like this:
 * >	jg	.+24			# offset 0
 *	larl	%r1,<&counter>		# offset 6
 *	brasl	%r14,_mcount		# offset 12
 *	lg	%r14,8(%r15)		# offset 18
 * The jg instruction branches to offset 24 to skip as many instructions
 * as possible.
 * In case we use gcc's hotpatch feature the original and also the disabled
 * function prologue contains only a single six byte instruction and looks
 * like this:
 * >	brcl	0,0			# offset 0
 * To enable ftrace the code gets patched like above and afterwards looks
 * like this:
 * >	brasl	%r0,ftrace_caller	# offset 0
 */

unsigned long ftrace_plt;

int ftrace_modify_call(struct dyn_ftrace *rec, unsigned long old_addr,
		       unsigned long addr)
{
	return 0;
}

int ftrace_make_nop(struct module *mod, struct dyn_ftrace *rec,
		    unsigned long addr)
{
	struct ftrace_insn orig, new, old;

	if (probe_kernel_read(&old, (void *) rec->ip, sizeof(old)))
		return -EFAULT;
	if (addr == MCOUNT_ADDR) {
		/* Initial code replacement */
#ifdef CC_USING_HOTPATCH
		/* We expect to see brcl 0,0 */
		ftrace_generate_nop_insn(&orig);
#else
		/* We expect to see stg r14,8(r15) */
		orig.opc = 0xe3e0;
		orig.disp = 0xf0080024;
#endif
		ftrace_generate_nop_insn(&new);
	} else if (old.opc == BREAKPOINT_INSTRUCTION) {
		/*
		 * If we find a breakpoint instruction, a kprobe has been
		 * placed at the beginning of the function. We write the
		 * constant KPROBE_ON_FTRACE_NOP into the remaining four
		 * bytes of the original instruction so that the kprobes
		 * handler can execute a nop, if it reaches this breakpoint.
		 */
		new.opc = orig.opc = BREAKPOINT_INSTRUCTION;
		orig.disp = KPROBE_ON_FTRACE_CALL;
		new.disp = KPROBE_ON_FTRACE_NOP;
	} else {
		/* Replace ftrace call with a nop. */
		ftrace_generate_call_insn(&orig, rec->ip);
		ftrace_generate_nop_insn(&new);
	}
	/* Verify that the to be replaced code matches what we expect. */
	if (memcmp(&orig, &old, sizeof(old)))
		return -EINVAL;
	if (probe_kernel_write((void *) rec->ip, &new, sizeof(new)))
		return -EPERM;
	return 0;
}

int ftrace_make_call(struct dyn_ftrace *rec, unsigned long addr)
{
	struct ftrace_insn orig, new, old;

	if (probe_kernel_read(&old, (void *) rec->ip, sizeof(old)))
		return -EFAULT;
	if (old.opc == BREAKPOINT_INSTRUCTION) {
		/*
		 * If we find a breakpoint instruction, a kprobe has been
		 * placed at the beginning of the function. We write the
		 * constant KPROBE_ON_FTRACE_CALL into the remaining four
		 * bytes of the original instruction so that the kprobes
		 * handler can execute a brasl if it reaches this breakpoint.
		 */
		new.opc = orig.opc = BREAKPOINT_INSTRUCTION;
		orig.disp = KPROBE_ON_FTRACE_NOP;
		new.disp = KPROBE_ON_FTRACE_CALL;
	} else {
		/* Replace nop with an ftrace call. */
		ftrace_generate_nop_insn(&orig);
		ftrace_generate_call_insn(&new, rec->ip);
	}
	/* Verify that the to be replaced code matches what we expect. */
	if (memcmp(&orig, &old, sizeof(old)))
		return -EINVAL;
	if (probe_kernel_write((void *) rec->ip, &new, sizeof(new)))
		return -EPERM;
	return 0;
}

int ftrace_update_ftrace_func(ftrace_func_t func)
{
	return 0;
}

int __init ftrace_dyn_arch_init(void *data)
{
	*(unsigned long *) data = 0;
	return 0;
}

static int __init ftrace_plt_init(void)
{
	unsigned int *ip;

	ftrace_plt = (unsigned long) module_alloc(PAGE_SIZE);
	if (!ftrace_plt)
		panic("cannot allocate ftrace plt\n");
	ip = (unsigned int *) ftrace_plt;
	ip[0] = 0x0d10e310; /* basr 1,0; lg 1,10(1); br 1 */
	ip[1] = 0x100a0004;
	ip[2] = 0x07f10000;
	ip[3] = FTRACE_ADDR >> 32;
	ip[4] = FTRACE_ADDR & 0xffffffff;
	set_memory_ro(ftrace_plt, 1);
	return 0;
}
device_initcall(ftrace_plt_init);

#ifdef CONFIG_FUNCTION_GRAPH_TRACER
/*
 * Hook the return address and push it in the stack of return addresses
 * in current thread info.
 */
unsigned long __kprobes prepare_ftrace_return(unsigned long parent,
					      unsigned long ip)
{
	struct ftrace_graph_ent trace;

	if (unlikely(atomic_read(&current->tracing_graph_pause)))
		goto out;
	ip = (ip & PSW_ADDR_INSN) - MCOUNT_INSN_SIZE;
	if (ftrace_push_return_trace(parent, ip, &trace.depth, 0) == -EBUSY)
		goto out;
	trace.func = ip;
	/* Only trace if the calling function expects to. */
	if (!ftrace_graph_entry(&trace)) {
		current->curr_ret_stack--;
		goto out;
	}
	parent = (unsigned long) return_to_handler;
out:
	return parent;
}

/*
 * Patch the kernel code at ftrace_graph_caller location. The instruction
 * there is branch relative on condition. To enable the ftrace graph code
 * block, we simply patch the mask field of the instruction to zero and
 * turn the instruction into a nop.
 * To disable the ftrace graph code the mask field will be patched to
 * all ones, which turns the instruction into an unconditional branch.
 */
int ftrace_enable_ftrace_graph_caller(void)
{
	u8 op = 0x04; /* set mask field to zero */

	return probe_kernel_write(__va(ftrace_graph_caller)+1, &op, sizeof(op));
}

int ftrace_disable_ftrace_graph_caller(void)
{
	u8 op = 0xf4; /* set mask field to all ones */

	return probe_kernel_write(__va(ftrace_graph_caller)+1, &op, sizeof(op));
}

#endif /* CONFIG_FUNCTION_GRAPH_TRACER */
