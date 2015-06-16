#ifndef _ASM_S390_FTRACE_H
#define _ASM_S390_FTRACE_H

#define ARCH_SUPPORTS_FTRACE_OPS 1

#ifdef CC_USING_HOTPATCH
#define MCOUNT_INSN_SIZE	6
#else
#define MCOUNT_INSN_SIZE	24
#define MCOUNT_RETURN_FIXUP	18
#endif

#define MCOUNT_OCO_INSN_SIZE	24
#define MCOUNT_OCO_RETURN_FIXUP	18

/*
 * This is a SLES12 GA hack only:
 * After GA the ftrace code got changed to convert the first instead of
 * the second instruction. However we cannot change the recorded offsets
 * within the __mcount_loc section, due to out of tree built modules.
 * Therefore all (struct dyn_ftrace *) rec->ip passed instruction
 * addresses do not point to the instruction being changed, but six bytes
 * further.
 * The MCOUNT_IP_FIXUP define must be used everywhere to fixup the address.
 */
#define MCOUNT_IP_FIXUP		6

#ifndef __ASSEMBLY__

void _mcount(void);
void ftrace_caller(void);

extern char ftrace_graph_caller_end;
extern char ftrace_oco_graph_caller_end;
extern unsigned long ftrace_plt;
extern unsigned long ftrace_oco_plt;

void ftrace_oco_caller(void);
void ftrace_oco_graph_caller(void);

struct dyn_arch_ftrace { };

#define MCOUNT_ADDR ((unsigned long)_mcount)
#define FTRACE_ADDR ((unsigned long)ftrace_caller)
#define FTRACE_OCO_ADDR ((unsigned long)ftrace_oco_caller)

#define KPROBE_ON_FTRACE_NOP	0
#define KPROBE_ON_FTRACE_CALL	1
#define KPROBE_ON_FTRACE_OCO_NOP	2
#define KPROBE_ON_FTRACE_OCO_CALL	3

static inline unsigned long ftrace_call_adjust(unsigned long addr)
{
	return addr;
}

struct ftrace_insn {
	u16 opc;
	s32 disp;
} __packed;

static inline void ftrace_generate_nop_insn(struct ftrace_insn *insn, int oco)
{
#ifdef CONFIG_FUNCTION_TRACER
#ifdef CC_USING_HOTPATCH
	if (oco) {
		/* jg .+24 */
		insn->opc = 0xc0f4;
		insn->disp = MCOUNT_OCO_INSN_SIZE / 2;
	} else {
		/* brcl 0,0 */
		insn->opc = 0xc004;
		insn->disp = 0;
	}
#else
	/* jg .+24 */
	insn->opc = 0xc0f4;
	insn->disp = MCOUNT_INSN_SIZE / 2;
#endif
#endif
}

static inline int is_ftrace_nop(struct ftrace_insn *insn, unsigned long ip)
{
#ifdef CONFIG_FUNCTION_TRACER
#ifdef CC_USING_HOTPATCH
	if (insn->disp == 0)
		return 1;
#else
	if (insn->disp == MCOUNT_INSN_SIZE / 2)
		return 1;
#endif
#endif
	return 0;
}

static inline int is_ftrace_oco_orig(struct ftrace_insn *insn, unsigned long ip)
{
#ifdef CC_USING_HOTPATCH
	if (!is_module_addr((void *) ip))
		return 0;
	/* stg r14,8(r15) */
	if (insn->opc == 0xe3e0 && insn->disp == 0xf0080024)
		return 1;
#endif
	return 0;
}

static inline int is_ftrace_oco_nop(struct ftrace_insn *insn, unsigned long ip)
{
#ifdef CC_USING_HOTPATCH
	if (!is_module_addr((void *) ip))
		return 0;
	if (insn->disp == MCOUNT_OCO_INSN_SIZE / 2)
		return 1;
#endif
	return 0;
}

static inline int is_ftrace_call(struct ftrace_insn *insn, unsigned long ip)
{
	if (insn->disp == (s32)((ftrace_plt - ip) / 2))
		return 1;
	return 0;
}

static inline int is_ftrace_oco_call(struct ftrace_insn *insn, unsigned long ip)
{
#ifdef CC_USING_HOTPATCH
	if (!is_module_addr((void *) ip))
		return 0;
	if (insn->disp == (s32)((ftrace_oco_plt - ip) / 2))
		return 1;
#endif
	return 0;
}

static inline void ftrace_generate_call_insn(struct ftrace_insn *insn,
					     unsigned long ip, int oco)
{
#ifdef CONFIG_FUNCTION_TRACER
	unsigned long target;

	/* brasl r0,ftrace_caller */
	if (oco)
		target = is_module_addr((void *) ip) ? ftrace_oco_plt : FTRACE_ADDR;
	else
		target = is_module_addr((void *) ip) ? ftrace_plt : FTRACE_ADDR;
	insn->opc = 0xc005;
	insn->disp = (target - ip) / 2;
#endif
}

#endif /* __ASSEMBLY__ */
#endif /* _ASM_S390_FTRACE_H */
