/*
 * Kernel Debugger Architecture Independent Support Functions
 *
 * This file is subject to the terms and conditions of the GNU General Public
 * License.  See the file "COPYING" in the main directory of this archive
 * for more details.
 *
 * Copyright (c) 1999-2004 Silicon Graphics, Inc.  All Rights Reserved.
 */

#include <linux/kdb.h>
#include "../kdb_private.h"
#include "x86_64-kdbprivate.h"
#include <asm/kdebug.h>
#include <asm/processor.h>
#include <asm/msr.h>

typedef unsigned long kdb_machreg_t;

kdb_machreg_t kdba_getdr(int);
void kdba_putdr(int, kdb_machreg_t);



kdb_machreg_t
kdba_getdr6(void)
{
	return kdba_getdr(6);
}

kdb_machreg_t
kdba_getdr7(void)
{
	return kdba_getdr(7);
}

void
kdba_putdr6(kdb_machreg_t contents)
{
	kdba_putdr(6, contents);
}

kdb_machreg_t
kdba_getdr(int regnum)
{
	kdb_machreg_t contents = 0;
	switch(regnum) {
	case 0:
		__asm__ ("movq %%db0,%0\n\t":"=r"(contents));
		break;
	case 1:
		__asm__ ("movq %%db1,%0\n\t":"=r"(contents));
		break;
	case 2:
		__asm__ ("movq %%db2,%0\n\t":"=r"(contents));
		break;
	case 3:
		__asm__ ("movq %%db3,%0\n\t":"=r"(contents));
		break;
	case 4:
	case 5:
		break;
	case 6:
		__asm__ ("movq %%db6,%0\n\t":"=r"(contents));
		break;
	case 7:
		__asm__ ("movq %%db7,%0\n\t":"=r"(contents));
		break;
	default:
		break;
	}

	return contents;
}


kdb_machreg_t
kdb_getcr(int regnum)
{
	kdb_machreg_t contents = 0;
	switch(regnum) {
	case 0:
		__asm__ ("movq %%cr0,%0\n\t":"=r"(contents));
		break;
	case 1:
		break;
	case 2:
		__asm__ ("movq %%cr2,%0\n\t":"=r"(contents));
		break;
	case 3:
		__asm__ ("movq %%cr3,%0\n\t":"=r"(contents));
		break;
	case 4:
		__asm__ ("movq %%cr4,%0\n\t":"=r"(contents));
		break;
	default:
		break;
	}

	return contents;
}

void
kdba_putdr(int regnum, kdb_machreg_t contents)
{
	switch(regnum) {
	case 0:
		__asm__ ("movq %0,%%db0\n\t"::"r"(contents));
		break;
	case 1:
		__asm__ ("movq %0,%%db1\n\t"::"r"(contents));
		break;
	case 2:
		__asm__ ("movq %0,%%db2\n\t"::"r"(contents));
		break;
	case 3:
		__asm__ ("movq %0,%%db3\n\t"::"r"(contents));
		break;
	case 4:
	case 5:
		break;
	case 6:
		__asm__ ("movq %0,%%db6\n\t"::"r"(contents));
		break;
	case 7:
		__asm__ ("movq %0,%%db7\n\t"::"r"(contents));
		break;
	default:
		break;
	}
}

/*
 * kdba_getregcontents
 *
 *	Return the contents of the register specified by the
 *	input string argument.   Return an error if the string
 *	does not match a machine register.
 *
 *	The following pseudo register names are supported:
 *	   &regs	 - Prints address of exception frame
 *	   krsp		 - Prints kernel stack pointer at time of fault
 *	   crsp		 - Prints current kernel stack pointer, inside kdb
 *	   ceflags	 - Prints current flags, inside kdb
 *	   %<regname>	 - Uses the value of the registers at the
 *			   last time the user process entered kernel
 *			   mode, instead of the registers at the time
 *			   kdb was entered.
 *
 * Parameters:
 *	regname		Pointer to string naming register
 *	regs		Pointer to structure containing registers.
 * Outputs:
 *	*contents	Pointer to unsigned long to recieve register contents
 * Returns:
 *	0		Success
 *	KDB_BADREG	Invalid register name
 * Locking:
 * 	None.
 * Remarks:
 * 	If kdb was entered via an interrupt from the kernel itself then
 *	ss and rsp are *not* on the stack.
 */

static struct kdbregs {
	char   *reg_name;
	size_t	reg_offset;
} kdbreglist[] = {
	{ "r15",	offsetof(struct pt_regs, r15) },
	{ "r14",	offsetof(struct pt_regs, r14) },
	{ "r13",	offsetof(struct pt_regs, r13) },
	{ "r12",	offsetof(struct pt_regs, r12) },
	{ "rbp",	offsetof(struct pt_regs, bp) },
	{ "rbx",	offsetof(struct pt_regs, bx) },
	{ "r11",	offsetof(struct pt_regs, r11) },
	{ "r10",	offsetof(struct pt_regs, r10) },
	{ "r9",		offsetof(struct pt_regs, r9) },
	{ "r8",		offsetof(struct pt_regs, r8) },
	{ "rax",	offsetof(struct pt_regs, ax) },
	{ "rcx",	offsetof(struct pt_regs, cx) },
	{ "rdx",	offsetof(struct pt_regs, dx) },
	{ "rsi",	offsetof(struct pt_regs, si) },
	{ "rdi",	offsetof(struct pt_regs, di) },
	{ "orig_rax",	offsetof(struct pt_regs, orig_ax) },
	{ "rip",	offsetof(struct pt_regs, ip) },
	{ "cs",		offsetof(struct pt_regs, cs) },
	{ "eflags", 	offsetof(struct pt_regs, flags) },
	{ "rsp",	offsetof(struct pt_regs, sp) },
	{ "ss",		offsetof(struct pt_regs, ss) },
};

static const int nkdbreglist = sizeof(kdbreglist) / sizeof(struct kdbregs);

static struct kdbregs dbreglist[] = {
	{ "dr0", 	0 },
	{ "dr1", 	1 },
	{ "dr2",	2 },
	{ "dr3", 	3 },
	{ "dr6", 	6 },
	{ "dr7",	7 },
};

static const int ndbreglist = sizeof(dbreglist) / sizeof(struct kdbregs);

int
kdba_getregcontents(const char *regname,
		    struct pt_regs *regs,
		    kdb_machreg_t *contents)
{
	int i;

	if (strcmp(regname, "&regs") == 0) {
		*contents = (unsigned long)regs;
		return 0;
	}

	if (strcmp(regname, "krsp") == 0) {
		*contents = (unsigned long)regs + sizeof(struct pt_regs);
		if ((regs->cs & 0xffff) == __KERNEL_CS) {
			/* rsp and ss are not on stack */
			*contents -= 2*4;
		}
		return 0;
	}

	if (strcmp(regname, "crsp") == 0) {
		asm volatile("movq %%rsp,%0":"=m" (*contents));
		return 0;
	}

	if (strcmp(regname, "ceflags") == 0) {
		unsigned long flags;
		local_save_flags(flags);
		*contents = flags;
		return 0;
	}

	if (regname[0] == '%') {
		/* User registers:  %%r[a-c]x, etc */
		regname++;
		regs = (struct pt_regs *)
#ifdef CONFIG_X86_32            
			(current->thread.sp0 - sizeof(struct pt_regs));
#else        
			(current->thread.sp - sizeof(struct pt_regs));
#endif            
	}

	for (i=0; i<nkdbreglist; i++) {
		if (strncasecmp(kdbreglist[i].reg_name,
			     regname,
			     strlen(regname)) == 0)
			break;
	}

	if ((i < nkdbreglist)
	 && (strlen(kdbreglist[i].reg_name) == strlen(regname))) {
		if ((regs->cs & 0xffff) == __KERNEL_CS) {
			/* No cpl switch, rsp is not on stack */
			if (strcmp(kdbreglist[i].reg_name, "rsp") == 0) {
				*contents = (kdb_machreg_t)regs +
					sizeof(struct pt_regs) - 2*8;
				return(0);
			}
		}
		*contents = *(unsigned long *)((unsigned long)regs +
				kdbreglist[i].reg_offset);
		return(0);
	}

	for (i=0; i<ndbreglist; i++) {
		if (strncasecmp(dbreglist[i].reg_name,
			     regname,
			     strlen(regname)) == 0)
			break;
	}

	if ((i < ndbreglist)
	 && (strlen(dbreglist[i].reg_name) == strlen(regname))) {
		*contents = kdba_getdr(dbreglist[i].reg_offset);
		return 0;
	}
	return KDB_BADREG;
}

/*
 * kdba_setregcontents
 *
 *	Set the contents of the register specified by the
 *	input string argument.   Return an error if the string
 *	does not match a machine register.
 *
 *	Supports modification of user-mode registers via
 *	%<register-name>
 *
 * Parameters:
 *	regname		Pointer to string naming register
 *	regs		Pointer to structure containing registers.
 *	contents	Unsigned long containing new register contents
 * Outputs:
 * Returns:
 *	0		Success
 *	KDB_BADREG	Invalid register name
 * Locking:
 * 	None.
 * Remarks:
 */

int
kdba_setregcontents(const char *regname,
		  struct pt_regs *regs,
		  unsigned long contents)
{
	int i;

	if (regname[0] == '%') {
		regname++;
		regs = (struct pt_regs *)
#ifdef CONFIG_X86_32            
                        (current->thread.sp0 - sizeof(struct pt_regs));
#else        
                        (current->thread.sp - sizeof(struct pt_regs));
#endif            
	}

	for (i=0; i<nkdbreglist; i++) {
		if (strncasecmp(kdbreglist[i].reg_name,
			     regname,
			     strlen(regname)) == 0)
			break;
	}

	if ((i < nkdbreglist)
	 && (strlen(kdbreglist[i].reg_name) == strlen(regname))) {
		*(unsigned long *)((unsigned long)regs
				   + kdbreglist[i].reg_offset) = contents;
		return 0;
	}

	for (i=0; i<ndbreglist; i++) {
		if (strncasecmp(dbreglist[i].reg_name,
			     regname,
			     strlen(regname)) == 0)
			break;
	}

	if ((i < ndbreglist)
	 && (strlen(dbreglist[i].reg_name) == strlen(regname))) {
		kdba_putdr(dbreglist[i].reg_offset, contents);
		return 0;
	}

	return KDB_BADREG;
}

/*
 * kdba_dumpregs
 *
 *	Dump the specified register set to the display.
 *
 * Parameters:
 *	regs		Pointer to structure containing registers.
 *	type		Character string identifying register set to dump
 *	extra		string further identifying register (optional)
 * Outputs:
 * Returns:
 *	0		Success
 * Locking:
 * 	None.
 * Remarks:
 *	This function will dump the general register set if the type
 *	argument is NULL (struct pt_regs).   The alternate register
 *	set types supported by this function:
 *
 *	d 		Debug registers
 *	c		Control registers
 *	u		User registers at most recent entry to kernel
 * Following not yet implemented:
 *	m		Model Specific Registers (extra defines register #)
 *	r		Memory Type Range Registers (extra defines register)
 */

int
kdba_dumpregs(struct pt_regs *regs,
	    const char *type,
	    const char *extra)
{
	int i;
	int count = 0;

	if (type
	 && (type[0] == 'u')) {
		type = NULL;
		regs = (struct pt_regs *)
#ifdef CONFIG_X86_32            
            (current->thread.sp0 - sizeof(struct pt_regs));
#else        
            (current->thread.sp - sizeof(struct pt_regs));
#endif            
	}

	if (type == NULL) {
		struct kdbregs *rlp;
		kdb_machreg_t contents;

		for (i=0, rlp=kdbreglist; i<nkdbreglist; i++,rlp++) {
			kdb_printf("%8s = ", rlp->reg_name);
			kdba_getregcontents(rlp->reg_name, regs, &contents);
			kdb_printf("0x%016lx ", contents);
			if ((++count % 2) == 0)
				kdb_printf("\n");
		}

		kdb_printf("&regs = 0x%p\n", regs);

		return 0;
	}

	switch (type[0]) {
	case 'd':
	{
		unsigned long dr[8];

		for(i=0; i<8; i++) {
			if ((i == 4) || (i == 5)) continue;
			dr[i] = kdba_getdr(i);
		}
		kdb_printf("dr0 = 0x%08lx  dr1 = 0x%08lx  dr2 = 0x%08lx  dr3 = 0x%08lx\n",
			   dr[0], dr[1], dr[2], dr[3]);
		kdb_printf("dr6 = 0x%08lx  dr7 = 0x%08lx\n",
			   dr[6], dr[7]);
		return 0;
	}
	case 'c':
	{
		unsigned long cr[5];

		for (i=0; i<5; i++) {
			cr[i] = kdb_getcr(i);
		}
		kdb_printf("cr0 = 0x%08lx  cr1 = 0x%08lx  cr2 = 0x%08lx  cr3 = 0x%08lx\ncr4 = 0x%08lx\n",
			   cr[0], cr[1], cr[2], cr[3], cr[4]);
		return 0;
	}
	case 'm':
		break;
	case 'r':
		break;
	default:
		return KDB_BADREG;
	}

	/* NOTREACHED */
	return 0;
}
