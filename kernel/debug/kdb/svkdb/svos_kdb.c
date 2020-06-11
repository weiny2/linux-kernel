#include <linux/module.h>
#include <linux/pci.h>
#include <linux/kdb.h>
#include "../kdb_private.h"
#include <linux/delay.h>
#include <linux/ctype.h>
#include <linux/kernel.h>
#include <linux/file.h>
#include <linux/console.h>
#include <asm/processor.h>
#include <asm/irq_regs.h>
#ifdef CONFIG_X86
#include <asm/msr.h>
#include <asm/apic.h>
#include <asm/apicdef.h>

extern struct pt_regs *kdb_current_regs;

#define KDB_FUNCTION (kdb_func_t)

typedef unsigned long kdb_machreg_t;
extern void kdb_id1(unsigned long);

static const char * const _x86_cap_flags[32*NCAPINTS] = {
	/* Intel-defined */
#ifndef CONFIG_X86_64
	"fpu", "vme", "de", "pse", "tsc", "msr", "pae", "mce",
	"cx8", "apic", NULL, "sep", "mtrr", "pge", "mca", "cmov",
	"pat", "pse36", "pn", "clflush", NULL, "dts", "acpi", "mmx",
	"fxsr", "sse", "sse2", "ss", "ht", "tm", "ia64", NULL,
#else
	"fpu", "vme", "de", "pse", "tsc", "msr", "pae", "mce",
	"cx8", "apic", NULL, "sep", "mtrr", "pge", "mca", "cmov",
	"pat", "pse36", "pn", "clflush", NULL, "dts", "acpi", "mmx",
	"fxsr", "sse", "sse2", "ss", "ht", "tm", "ia64", "pbe",
#endif
	/* AMD-defined */
	NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
	NULL, NULL, NULL, "syscall", NULL, NULL, NULL, NULL,
	NULL, NULL, NULL, NULL, NULL, NULL, "mmxext", NULL,
	NULL, NULL, NULL, NULL, NULL, "lm", "3dnowext", "3dnow",

	/* Transmeta-defined */
	"recovery", "longrun", NULL, "lrti", NULL, NULL, NULL, NULL,
	NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
	NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
	NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,

	/* Other (Linux-defined) */
	"cxmmx", "k6_mtrr", "cyrix_arr", "centaur_mcr", NULL, NULL, NULL, NULL,
	NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
	NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
	NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
#ifdef CONFIG_X86_64
	/* Intel Defined (cpuid 1 and ecx) */
	"pni", NULL, NULL, "monitor", "ds-cpl", "vmx", "smx", "est",
	"tm2", "ssse3", "cid", NULL, NULL, "cmpxchg16b", "xtpr", NULL,
#ifndef CONFIG_XXAPIC
	NULL, NULL, "dca", NULL, NULL, NULL, NULL, "popcnt",
#else
	NULL, NULL, "dca", NULL, NULL, "xxapic", NULL, "popcnt",
#endif
	NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
#endif

};
#ifdef CONFIG_X86_64
static char *_x86_power_flags[] = {
	"ts",   /* temperature sensor */
	"fid",  /* frequency id control */
	"vid",  /* voltage id control */
	"ttp",  /* thermal trip */
};
#endif


//
// wrappers for kdb functions which are not exported in the 3.6 kernel
//  or need GPL licensing to use.
// 
int svfs_kdb_register( char *cmd, kdb_func_t func, char *usage, char *help, short minlen)
{
	return kdb_register( cmd, func, usage, help, minlen);
}
EXPORT_SYMBOL(svfs_kdb_register);

int svfs_kdb_unregister( char *cmd )
{
	return kdb_unregister( cmd );
}
EXPORT_SYMBOL(svfs_kdb_unregister);

int svfs_kdb_printf( const char *fmt, ... )
{
	va_list	ap;
	int 	r;

	va_start(ap, fmt );
	r = kdb_printf( fmt, ap );
	va_end( ap );

	return r;
}
EXPORT_SYMBOL(svfs_kdb_printf);

void svfs_kgdb_breakpoint(void)
{
	kgdb_breakpoint();
}
EXPORT_SYMBOL(svfs_kgdb_breakpoint);

int svfs_kdbgetularg(const char *arg, unsigned long *value)
{
	return kdbgetularg( arg,  value);
}
EXPORT_SYMBOL(svfs_kdbgetularg);

int svfs_kdbgetaddrarg(int argc, const char **argv, int*nextarg, unsigned long *value,
			 long *offset, char **name)
{
	return kdbgetaddrarg(  argc, argv, nextarg, value, offset, name);
}
EXPORT_SYMBOL(svfs_kdbgetaddrarg);

int svfs_kdbnearsym(unsigned long addr, kdb_symtab_t *symtab)
{
	return kdbnearsym( addr, symtab );
}
EXPORT_SYMBOL(svfs_kdbnearsym);

void svfs_kdb_symbol_print(unsigned long addr,  unsigned int punc)
{
	kdb_symbol_print( addr, NULL,  punc );
}
EXPORT_SYMBOL(svfs_kdb_symbol_print);

int
kdb_cpuid(int argc, const char **argv, const char **envp, struct pt_regs *regs)
{
	int cpu=smp_processor_id();
	int i;
	struct cpuinfo_x86 *c = &(cpu_data(cpu));

	kdb_printf("processor	: %d\n",cpu);
	kdb_printf("vendor_id	: %s\n",c->x86_vendor_id);
	kdb_printf("cpu family       : %d\n",c->x86);
	kdb_printf("model	    : %x\n",c->x86_model);
	kdb_printf("model name       : %s\n",c->x86_model_id);
	if(c->x86_stepping || c->cpuid_level >= 0)
		kdb_printf("stepping	 : %d\n",c->x86_stepping);
	else
		kdb_printf("stepping	 : unknown\n");
	if (test_bit(X86_FEATURE_TSC, (long unsigned int *)c->x86_capability)){
	//if (test_bit(X86_FEATURE_TSC, &c->x86_capability)){
		kdb_printf("cpu MHZ	  : %lu.%03lu\n",
			(long unsigned int)cpu_khz/1000,
			(long unsigned int)(cpu_khz%1000));
	}
	if (c->x86_cache_size >= 0)
		kdb_printf("cache size       : %d\n",c->x86_cache_size);
#ifndef CONFIG_X86_64
	kdb_printf("fdiv_bug	 : %s\n"
			"f00f_bug	 : %s\n"
			"coma_bug	 : %s\n"
			"fpu	      : %s\n"
			"cpuid level      : %d\n"
			"flags	    :",
			static_cpu_has_bug(X86_BUG_FDIV) ? "yes" : "no",
			static_cpu_has_bug(X86_BUG_F00F) ? "yes" : "no",
			static_cpu_has_bug(X86_BUG_COMA) ? "yes" : "no",
			boot_cpu_has(X86_FEATURE_FPU) ? "yes" : "no",
			c->cpuid_level);
#else
#ifdef CONFIG_SMP
	kdb_printf("physical id      : %d\n",c->phys_proc_id);
	kdb_printf("siblings	 : %d\n",smp_num_siblings);
#endif
	kdb_printf(
			"fpu	      : yes\n"
			"fpu_exception    : yes\n"
			"cpuid level      : %d\n"
			"wp	       : yes\n"
			"flags	    :",
			c->cpuid_level);
#endif
	for ( i = 0 ; i < 32*NCAPINTS ; i++ )
		if ( test_bit(i, (long unsigned int *)c->x86_capability) && _x86_cap_flags[i] != NULL )
			kdb_printf(" %s", _x86_cap_flags[i]);

	kdb_printf("\nbogomips	 : %lu.%02lu\n",
			c->loops_per_jiffy/(500000/HZ),
			(c->loops_per_jiffy/(5000/HZ)) % 100);
#ifdef CONFIG_X86_64
	if (c->x86_tlbsize > 0)
		kdb_printf("TLB size	 : %d 4K pages\n", c->x86_tlbsize);
	kdb_printf("clflush size     : %d\n", c->x86_clflush_size);

	if (c->x86_phys_bits > 0)
		kdb_printf("address sizes    : %u bits physical, %u bits virtual\n",
			c->x86_phys_bits, c->x86_virt_bits);

	kdb_printf("power management :");
	{
		for (i = 0; i < 32; i++)
			if (c->x86_power & (1 << i)) {
				if (i < ARRAY_SIZE(_x86_power_flags))
					kdb_printf(" %s", _x86_power_flags[i]);
				else
					kdb_printf(" [%d]", i);
		}
	}
	kdb_printf("\n\n");
#endif
	return 0;
}

int 
kdb_bpC(int argc, const char **argv, const char **envp, struct pt_regs *regs)
{
	extern int sv_kdb_bc(int argc, const char **argv);
	
	const char *cmdargv[] ={ "bc", "*" };
	return(sv_kdb_bc(1,cmdargv));
}

int
kdb_bpD(int argc, const char **argv, const char **envp, struct pt_regs *regs)
{
	extern int sv_kdb_bc(int argc, const char **argv);	

	const char *cmdargv[]={"bd","*"};
	return(sv_kdb_bc(1,cmdargv));
}

int
kdb_bpE(int argc, const char **argv, const char **envp, struct pt_regs *regs)
{
	extern int sv_kdb_bc(int argc, const char **argv);	

	const char* cmdargv[]={"be","*"};
	return(sv_kdb_bc(1,cmdargv));
}

int SSRFLAG=0;
unsigned long ADDRFLAG=0;
int SOUT_PCFLAG=0;
int SOUT_CALLFLAG=0;
int SOFLAG=0;
int SO_SFLAG=0;
int SORFLAG=0;
int PCFLAG=0;
int SOUTFLAG=0;
char opcode[50]="";
extern int sv_kdb_go(int argc, const char **argv);

int
kdb_ssr(int argc, const char **argv, const char **envp, struct pt_regs *regs)
{
	extern int sv_kdb_ss(int argc,const char **argv);	

	const char* cmdargv[]={"ss"};
	SSRFLAG=1;
	
	return(sv_kdb_ss(0,cmdargv));
}

extern int sv_kdb_bc( int, const char ** );

void
kdb_so_end( unsigned long ip )
{
	char	cmdbuff[32];
	const char	*cmdarg[] = {"bq", cmdbuff};

	memset(cmdbuff, 0, sizeof(cmdbuff));
	sprintf(cmdbuff, "%lx", ip);
	cmdarg[0] = "bq";
	cmdarg[1] = cmdbuff;
	sv_kdb_bc(1, cmdarg );
} 

int
kdb_so(int argc, const char **argv, const char **envp)
{

	struct pt_regs *regs = kdb_current_regs;
	extern int sv_kdb_ss(int argc, const char **argv); 
	extern int sv_kdb_bp(int argc, const char **argv);
	const char* cmdargv2[]={"bp",""};
	const char* cmdargv3[]={"go"};
	char temp[50];
	unsigned long ip_addr;	

	if (argc > 1) {
		return KDB_ARGCOUNT;
	}

#ifdef CONFIG_X86_64
	ip_addr=instruction_pointer( regs );
#else
      	ip_addr=regs->ip;
#endif
	SOFLAG=1;
	kdb_printf("Stepping over ");
	kdb_id1(ip_addr);

	if(strcmp(argv[0],"sor")==0)
		SORFLAG=1;

	ADDRFLAG=(ip_addr)+PCFLAG;
	sprintf(temp,"%016lX",ADDRFLAG);
	cmdargv2[1]=temp;
	sv_kdb_bp(1,cmdargv2);
	return sv_kdb_go(0,cmdargv3);
	return 0;
}

int
kdb_sout(int argc, const char **argv, const char **envp, struct pt_regs *regs)
{
	extern int sv_kdb_bp(int argc, const char **argv);
	extern int sv_kdb_ss(int argc, const char **argv);

	if(SOUTFLAG){
		const char* cmdargv2[]={"bp",""};
		const char* cmdargv3[]={"go"};
		char temp[50];
		
		SOFLAG=1;

		if(strcmp(argv[0],"soutr")==0)
			SORFLAG=1;

		sprintf(temp,"%016lX",ADDRFLAG);
		cmdargv2[1]=temp;
		sv_kdb_bp(1,cmdargv2);
		return sv_kdb_go(0,cmdargv3);
	}
	kdb_printf("A call function has not been stepped into.\n");
	return 0;
}

static char buf[40];

char 
*ascii_conversion(char s[],int search)
{
	int i;
	char temp[32];
	sprintf(buf,"0x");
	
	for(i=1;s[i]!='\0'||i<strlen(s);++i)
	{
		if(s[i]=='\"'){
			break;
		}
		sprintf(temp,"%x",s[i]);
		strncat(buf,temp,2);
	}
	return buf;
}

int 
kdb_mf(int argc, const char **argv, const char **envp, struct pt_regs *regs)
{
	int		diag;
	kdb_machreg_t	addr;
	long		offset = 0;
	unsigned long	contents;
	int		nextarg;
	int		width;
	int		times,i;
	int		bytesperword = sizeof(kdb_machreg_t);
	int		length;
	char		pattern[16];
	kdb_machreg_t	val = 0;
	kdbgetintenv("BYTESPERWORD", &bytesperword);

	if (argv[0][2] && !isdigit(argv[0][2]))
		return KDB_NOTFOUND;

	if (argc < 2||argc > 3) {
		return KDB_ARGCOUNT;
	}
	nextarg = 1;
	if ((diag = kdbgetaddrarg(argc, argv, &nextarg, &addr, &offset, NULL)))
		return diag;

	if (nextarg > argc)
		return KDB_ARGCOUNT;
	
	length = strlen(argv[nextarg]);

	if(argv[nextarg][0]=='\"'){
			if(length>10){
				kdb_printf("String is too long.\n");
				return 0;
			}
			strcpy(pattern,ascii_conversion((char *)argv[nextarg],0));

			nextarg+=1;		
			if((diag = kdbgetularg(pattern, &contents))){
				return diag;
			}
	}
	else if((diag = kdbgetaddrarg(argc, argv, &nextarg, &contents, NULL, NULL)))
		return diag;

       	if(argv[nextarg]!=NULL){
		kdbgetularg(argv[nextarg], &val);
		times=(int) val;

	} else {
		times = 1;
	}
	
	for(i=0;i<times;i++){
		width = argv[0][2] ? (argv[0][2] - '0') : (sizeof(kdb_machreg_t));

		if ((diag = kdb_putword(addr, contents, width)))
			return(diag);
		kdb_printf(kdb_machreg_fmt " = " kdb_machreg_fmt "\n", addr, contents);

		switch (bytesperword) {
				case 8:
					addr += 4;
					/* fall through */
				case 4:
					addr += 2;
					/* fall through */
				case 2:
					addr++;
					/* fall through */
				case 1:
					addr++;
					break;
				}
	}
	return 0;
}

//
// cloned copy of strtok for use here regular strtok 
// has been depreciated.
//
static char *___strtok;
static char *sv_strtok( char *s, const char *ct)
{
       char *sbegin, *send;

	sbegin  = s ? s : ___strtok;
	if (!sbegin) {
		return NULL;
	}
	sbegin += strspn(sbegin,ct);
	if (*sbegin == '\0') {
		___strtok = NULL;
		return( NULL );
	}
	send = strpbrk( sbegin, ct);
	if (send && *send != '\0')
		*send++ = '\0';
	___strtok = send;
	return (sbegin);
}

int
kdb_ms(int argc, const char **argv, const char **envp, struct pt_regs *regs)
{	
	static kdb_machreg_t last_addr;
	static int last_radix, last_bytesperword, last_repeat;
	static char last_pattern[32];
	int radix = 16, mdcount = 8, bytesperword = sizeof(kdb_machreg_t), repeat;
	int nosect = 0;
	char fmtchar, fmtstr[64];
	kdb_machreg_t addr;
	unsigned long word;
	long offset = 0;
	kdb_symtab_t symtab;
	int symbolic = 0;
	int valid = 0;
	char pattern[]="default_pattern_default_pattern";
	int length;	

	kdbgetintenv("MDCOUNT", &mdcount);
	kdbgetintenv("RADIX", &radix);
	kdbgetintenv("BYTESPERWORD", &bytesperword);

	/* Assume 'md <addr>' and start with environment values */
	repeat = mdcount * 16 / bytesperword;

	if (isdigit(argv[0][2])) {
		bytesperword = (int)(argv[0][2] - '0');
		if (bytesperword == 0) {
			bytesperword = last_bytesperword;
			if (bytesperword == 0) {
				bytesperword = 4;
			}
		}
		last_bytesperword = bytesperword;
		repeat = mdcount * 16 / bytesperword;
		if (!argv[0][3])
			valid = 1;
		else if (argv[0][3] == 'c' && argv[0][4]) {
			char *p;
			repeat = simple_strtoul(argv[0]+4, &p, 10);
			mdcount = ((repeat * bytesperword) + 15) / 16;
			valid = !*p;
		}
		last_repeat = repeat;
	}
	else if (strcmp(argv[0], "ms") == 0)
		valid = 1;
	if (!valid)
		return KDB_NOTFOUND;
	if (argc == 0) {
		if (last_addr == 0)
			return KDB_ARGCOUNT;
		addr = last_addr;
		radix = last_radix;
		bytesperword = last_bytesperword;
		repeat = last_repeat;
		mdcount = ((repeat * bytesperword) + 15) / 16;
		strcpy(pattern,last_pattern);
	}

	if (argc) {
		kdb_machreg_t val;
		int diag, nextarg = 1;
		diag = kdbgetaddrarg(argc, argv, &nextarg, &addr, &offset, NULL);
		if (diag)
			return diag;
		if (argc > nextarg+2)
			return KDB_ARGCOUNT;
		if(argc >=nextarg){
			strcpy(pattern,argv[nextarg]);
			kdb_printf("Pattern: %s\n",pattern);
			length = strlen(argv[nextarg]);
			if(pattern[0]=='\"'){
				if(length>10){
					kdb_printf("String is too long.\n");
					return 0;
				}
					kdb_printf("%s=",pattern);
					strcpy(pattern,ascii_conversion((char *)argv[nextarg],1));
					kdb_printf("%s\n",pattern);
			}
			if(pattern[0]=='0'&&pattern[1]=='x'){
				strcpy(pattern,sv_strtok(pattern, "0x"));
			}
			strcpy(last_pattern,pattern);
		}
		if (argc >= nextarg+1) {
			diag = kdbgetularg(argv[nextarg+1], &val);
			if (!diag) {
				mdcount = (int) val;
				repeat = mdcount * 16 / bytesperword;
			}
	      }
		if (argc >= nextarg+2) {
			diag = kdbgetularg(argv[nextarg+2], &val);
			if (!diag)
				radix = (int) val;
		}
	}

	switch (radix) {
	case 10:
		fmtchar = 'd';
		break;
	case 16:
		fmtchar = 'x';
		break;
	case 8:
		fmtchar = 'o';
		break;
	default:
		return KDB_BADRADIX;
	}

	last_radix = radix;

	if (bytesperword > sizeof(kdb_machreg_t))
		return KDB_BADWIDTH;

	switch (bytesperword) {
	case 8:
		sprintf(fmtstr, "%%16.16l%c ", fmtchar);
		break;
	case 4:
		sprintf(fmtstr, "%%8.8l%c ", fmtchar);
		break;
	case 2:
		sprintf(fmtstr, "%%4.4l%c ", fmtchar);
		break;
	case 1:
		sprintf(fmtstr, "%%2.2l%c ", fmtchar);
		break;
	default:
		return KDB_BADWIDTH;
	}
	last_repeat = repeat;
	last_bytesperword = bytesperword;

	if (strcmp(argv[0], "mds") == 0) {
		symbolic = 1;
		/* Do not save these changes as last_*, they are temporary mds
		 * overrides.
		 */
		bytesperword = sizeof(kdb_machreg_t);
		repeat = mdcount;
		kdbgetintenv("NOSECT", &nosect);
	}

	/* Round address down modulo BYTESPERWORD */

	addr &= ~(bytesperword-1);

	while (repeat > 0) {
		int     num = (symbolic?1 :(16 / bytesperword));
		char    cbuf[32];
		char    *c = cbuf;
		int     i;
		char    *found,temp[64];
		int 	prt=0;
		char svos_patterns[200];
		char mem_loc[50];

		memset(cbuf, '\0', sizeof(cbuf));
	       	sprintf(mem_loc,kdb_machreg_fmt0" ",addr);
		strcpy(svos_patterns,mem_loc);
		for(i = 0; i < num && repeat--; i++) {
			if (kdb_getword(&word, addr, bytesperword))
				return 0;
			
			sprintf(temp,fmtstr,word);
			found=strstr(temp,pattern);
			strcat(svos_patterns,temp);
			
			if(found!=NULL){
				prt=1;
			} 
			if (symbolic) {
				kdbnearsym(word, &symtab);
			}
			else {
				memset(&symtab, 0, sizeof(symtab));
			}
			
			if (symtab.sym_name) {
				kdb_symbol_print(word, &symtab, 0);
				if (!nosect) {
					kdb_printf("\n");
					kdb_printf("		       %s %s "
						   kdb_machreg_fmt " " kdb_machreg_fmt " " kdb_machreg_fmt,
						symtab.mod_name,
						symtab.sec_name,
						symtab.sec_start,
						symtab.sym_start,
						symtab.sym_end);
				}
				addr += bytesperword;
			} else {
				union {
					u64 word;
					unsigned char c[8];
				} wc;
				unsigned char *cp = wc.c + 8 - bytesperword;
				wc.word = cpu_to_be64(word);
#define printable_char(c) ({unsigned char __c = c; isprint(__c) ? __c : '.';})
				switch (bytesperword) {
				case 8:
					*c++ = printable_char(*cp++);
					*c++ = printable_char(*cp++);
					*c++ = printable_char(*cp++);
					*c++ = printable_char(*cp++);
					addr += 4;
					/* fall through */
				case 4:
					*c++ = printable_char(*cp++);
					*c++ = printable_char(*cp++);
					addr += 2;
					/* fall through */
				case 2:
					*c++ = printable_char(*cp++);
					addr++;
					/* fall through */
				case 1:
					*c++ = printable_char(*cp++);
					addr++;
					break;
				}
#undef printable_char
			}
		}
			if(prt==1){
			kdb_printf("%s%*s %s\n",svos_patterns, (int)((num-i)*(2*bytesperword + 1)+1), " ", cbuf);
		}
		prt=0;
	}
	last_addr = addr;

	return 0;
}

int
svKDBwrlapic(int argc, const char **argv, const char **envp, struct pt_regs *regs)
{
#ifndef CONFIG_XXAPIC
	unsigned long lreg = 0;
	unsigned long reg_value = 0;
	unsigned int val, error;

	if (argc < 2) {
		return(KDB_ARGCOUNT);
	}

	error = kdbgetularg(argv[1], &lreg);
	if (error != 0) {
		return(error);
	}
	error = kdbgetularg(argv[1], &reg_value);
	if (error != 0) {
		return(error);
	}
	val = reg_value;
	apic_write(lreg,val);
	return(0);
#else
	return KDB_BADMODE;
#endif
}

int
svKDBrdlapic(int argc, const char **argv, const char **envp, struct pt_regs *regs)
{
	unsigned int id, ldr, tpr, apr, ppr, dfr;
	unsigned int lint0, lint1, spiv;
	unsigned int timer;
	unsigned int error;
	unsigned int perfc;
	unsigned int isr0, isr1, isr2, isr3, isr4, isr5, isr6, isr7;
	unsigned int tmr0, tmr1, tmr2, tmr3, tmr4, tmr5, tmr6, tmr7;
	unsigned int irr0, irr1, irr2, irr3, irr4, irr5, irr6, irr7;
	unsigned int tmict, tdcr;

	id = apic_read(APIC_ID);
	ldr = apic_read(APIC_LDR);
	tpr = apic_read(APIC_TASKPRI);
	apr = apic_read(APIC_ARBPRI);
	ppr = apic_read(APIC_PROCPRI);
	dfr = apic_read(APIC_DFR);
	spiv = apic_read(APIC_SPIV);
	lint0 = apic_read(APIC_LVT0);
	lint1 = apic_read(APIC_LVT1);
	timer = apic_read(APIC_LVTT);
	error = apic_read(APIC_LVTERR);
	perfc = apic_read(APIC_LVTPC);
	isr0 = apic_read(0x0100);
	isr1 = apic_read(0x0110);
	isr2 = apic_read(0x0120);
	isr3 = apic_read(0x0130);
	isr4 = apic_read(0x0140);
	isr5 = apic_read(0x0150);
	isr6 = apic_read(0x0160);
	isr7 = apic_read(0x0170);
	tmr0 = apic_read(0x0180);
	tmr1 = apic_read(0x0190);
	tmr2 = apic_read(0x01a0);
	tmr3 = apic_read(0x01b0);
	tmr4 = apic_read(0x01c0);
	tmr5 = apic_read(0x01d0);
	tmr6 = apic_read(0x01e0);
	tmr7 = apic_read(0x01f0);
	irr0 = apic_read(0x0200);
	irr1 = apic_read(0x0210);
	irr2 = apic_read(0x0220);
	irr3 = apic_read(0x0230);
	irr4 = apic_read(0x0240);
	irr5 = apic_read(0x0250);
	irr6 = apic_read(0x0260);
	irr7 = apic_read(0x0270);
	tmict = apic_read(APIC_TMICT);
	tdcr = apic_read(APIC_TDCR);
	printk(KERN_WARNING "id %x ldr %x tpr %x apr %x ppr %x\n",id,ldr,tpr,apr,ppr);
	printk(KERN_WARNING "dfr %x spiv %x \n",dfr,spiv);
	printk(KERN_WARNING "l0 %x l1 %x tim %x err %x perfc %x \n",lint0, lint1, timer, error, perfc);
	printk(KERN_WARNING "isr %x %x %x %x %x %x %x %x \n",isr0, isr1, isr2, isr3,isr4,isr5,isr6,isr7);
	printk(KERN_WARNING "tmr %x %x %x %x %x %x %x %x \n",tmr0, tmr1, tmr2, tmr3,tmr4,tmr5,tmr6,tmr7);
	printk(KERN_WARNING "irr %x %x %x %x %x %x %x %x \n",irr0, irr1, irr2, irr3,irr4,irr5,irr6,irr7);
	printk(KERN_WARNING "tmict %x tdcr %x\n",tmict, tdcr);
	return(0);
}

int
svKDBio(int argc, const char **argv, const char **envp, struct pt_regs *regs)
{
	int	error;
	unsigned long	value;
	unsigned long	port;

	if (argc < 1) {
		return(KDB_ARGCOUNT);
	}

	error = kdbgetularg(argv[1], &port);
	if (error != 0) {
		return(error);
	}

	port &= 0xFFFF;

	if (argc > 1) {
		value = port;

		error = kdbgetularg(argv[2], &value);
		if (error != 0) {
			return(error);
		}

		if (argv[0][2] == 'b') {
			value &= 0xFF;
			kdb_printf("outb(%lX, %lX)\n", value, port);
			outb(value, port);
		} else if (argv[0][2] == 'w') {
			value &= 0xFFFF;
			kdb_printf("outw(%lX, %lX)\n", value, port);
			outw(value, port);
		} else {
			kdb_printf("outl(%lX, %lX)\n", value, port);
			outw(value, port);
		}
	} else {
		if (argv[0][2] == 'b') {
			kdb_printf("inb(%lX) = %X\n", port, inb(port));
		} else if (argv[0][2] == 'w') {
			kdb_printf("inw(%lX) = %X\n", port, inw(port));
		} else {
			kdb_printf("inl(%lX) = %X\n", port, inl(port));
		}
	}

	return(0);
}

int
svKDBkill(int argc, const char **argv, const char **envp, struct pt_regs *regs)
{
#ifdef RWCHG
	int	error;
	int	pid;
	int	signal;
	struct siginfo	info;

	if (argc != 2) {
		return(KDB_ARGCOUNT);
	}

	error = kdbgetularg(argv[1], (unsigned long *)&pid);
	if (error != 0) {
		return(error);
	}

	error = kdbgetularg(argv[2], (unsigned long *)&signal);
	if (error != 0) {
		return(error);
	}

	info.si_signo = signal;
	info.si_errno = 0;
	info.si_code = SI_USER;
	info.si_pid = current->pid;
	info.si_uid = current->real_cred->uid;

	error = kill_proc_info(signal, &info, pid);
	if (error < 0) {
		kdb_printf("Failed with errno %d\n", -error);
		return(KDB_BADINT);
	}
#else
	kdb_printf("Not implemented\n");
#endif

	return(0);
}

// This routine is unprotected. If an invalid msr argument is passed, the system
// will panic.
int
svKDBrdmsr(int argc, const char **argv, const char **envp, struct pt_regs *regs)
{
	unsigned long msr = 0;
	unsigned long msr_low = 0;
#ifndef CONFIG_X86_64
	unsigned long msr_high = 0;
#endif
	int	error;

	if (argc < 1) {
		return(KDB_ARGCOUNT);
	}
	error = kdbgetularg(argv[1], &msr);
	if (error != 0) {
		return(error);
	}
#ifdef CONFIG_X86_64
	rdmsrl(msr, msr_low);
	kdb_printf(" MSR 0x%x (%d)  %016lx\n", (unsigned int)msr, (int) msr, msr_low);
#else
	rdmsr(msr, msr_low, msr_high);
	kdb_printf(" MSR 0x%x (%d)  %08lx_%08lx\n", (unsigned int)msr, (int) msr, msr_high,msr_low);
#endif
	return(0);
}

// This routine is unprotected. If an invalid msr argument is passed, the system
// will panic. Bad data could panic the system as well.
int
svKDBwrmsr(int argc, const char **argv, const char **envp, struct pt_regs *regs)
{
	unsigned long msr = 0;
	unsigned long msr_low = 0;
#ifndef CONFIG_X86_64
	unsigned long msr_high = 0;
#endif
	int	error;

#ifdef CONFIG_X86_64
	if (argc < 2) {
		return(KDB_ARGCOUNT);
	}
#else
	if (argc < 3) {
		return(KDB_ARGCOUNT);
	}
#endif
	error = kdbgetularg(argv[1], &msr);
	if (error != 0) {
		return(error);
	}
#ifndef CONFIG_X86_64
	error = kdbgetularg(argv[2], &msr_high);
	if (error != 0) {
		return(error);
	}
	error = kdbgetularg(argv[3], &msr_low);
	if (error != 0) {
		return(error);
	}
#else
	error = kdbgetularg(argv[2], &msr_low);
	if (error != 0) {
		return(error);
	}
#ifdef DEBUG
	kdb_printf(" set low 0x%x \n", msr_low);
#endif
#endif //CONFIG_X86_64
#ifdef CONFIG_X86_64
#ifdef DEBUG
	kdb_printf(" set MSR 0x%x 0x%x\n", (unsigned int)msr, msr_low);
#endif
	wrmsrl(msr, msr_low);
#ifdef DEBUG
	kdb_printf(" After set MSR 0x%x 0x%x\n", (unsigned int)msr, msr_low);
#endif
#else
	wrmsr(msr, msr_low, msr_high);
#endif

	// Use this to do the display of what was written.
	return(svKDBrdmsr(argc, argv, envp, regs));
}

extern int kdb_id(int, const char **);
void 
svos_cmds_init(void)
{	
	// Quiet warnings about the console being locked
	atomic_inc(&ignore_console_lock_warning);
	kdb_register("iob", KDB_FUNCTION svKDBio, "<port> [<value>]",
				 "Read or write the given port", 0);
	kdb_register("iow", KDB_FUNCTION svKDBio, "<port> [<value>]",
				 "Read or write the given port", 0);
	kdb_register("iol", KDB_FUNCTION svKDBio, "<port> [<value>]",
				 "Read or write the given port", 0);
	kdb_register("rdmsr", KDB_FUNCTION svKDBrdmsr, "<msr#>",
				 "Read MSR (UNPROTECTED)", 0);
	kdb_register("wrmsr", KDB_FUNCTION svKDBwrmsr, "<msr#> <value>",
				 "Write MSR (UNPROTECTED)", 0);
	kdb_register("rdlapic", KDB_FUNCTION svKDBrdlapic, " ",
				 "local apic dump", 0);
	kdb_register("wrlapic", KDB_FUNCTION svKDBwrlapic, " ",
				 "write local apic <register> <value>", 0);
	kdb_register("kill", KDB_FUNCTION svKDBkill, "<pid> <signal>",
				 "Send the given signal to the given pid", 0);
	/*
	 *SVOS KDB Enhancements
	 */
	kdb_register_flags("id" , KDB_FUNCTION kdb_id, "<addr>",
				"disassemble instruction at address", 1, KDB_REPEAT_NO_ARGS);

	kdb_register("bpc", KDB_FUNCTION kdb_bpC,"",   "Clear All Breakpoint", 0);
	kdb_register("bpd", KDB_FUNCTION kdb_bpD,"",   "Disable All Breakpoint", 0);
    	kdb_register("bpe", KDB_FUNCTION kdb_bpE,"",   "Enable All Breakpoint", 0);
	kdb_register_flags("ssr", KDB_FUNCTION kdb_ssr, "", "Single Step With Register Dump", 1, KDB_REPEAT_NO_ARGS);
	kdb_register_flags("ms", KDB_FUNCTION kdb_ms, "<addr> <pattern> <length>","Search for <pattern> starting at <address>", 1, KDB_REPEAT_NO_ARGS);
	kdb_register_flags("mf", KDB_FUNCTION kdb_mf, "<addr> <pattern> <length>","Fill memory with <pattern> starting at <address>", 1, KDB_REPEAT_NO_ARGS);
	kdb_register_flags("so", KDB_FUNCTION kdb_so, "", "Step over a function call", 1, KDB_REPEAT_NO_ARGS);
	kdb_register_flags("sor", KDB_FUNCTION kdb_so, "", "Step over a function call with register dump", 1, KDB_REPEAT_NO_ARGS);
	kdb_register_flags("sout", KDB_FUNCTION kdb_sout, "", "Step out of function call", 1, KDB_REPEAT_NO_ARGS);
	kdb_register_flags("soutr", KDB_FUNCTION kdb_sout, "", "Step out of function call with register dump", 1, KDB_REPEAT_NO_ARGS);
	kdb_register_flags("cpuid", KDB_FUNCTION kdb_cpuid, "","Display the cpu model, family and features.", 1, KDB_REPEAT_NO_ARGS);
	atomic_dec(&ignore_console_lock_warning);
}

//
// Entry hook for svos
// called upon entry into kdb_local to allow for cleanup of any
// commands like the so command.
//
int
svos_entryhook( int reason, struct pt_regs *regs )
{
	if( SOFLAG ){
		SOFLAG = 0;
		kdb_so_end( instruction_pointer(regs));
		SOUTFLAG = 0;
		ADDRFLAG=-1;
	}
	return reason;
}

#else

#include <linux/kdb.h>
#include "dis-asm.h"

// Stubs for non-X86 builds

int PCFLAG=0;

void
svos_cmds_init(void)
{
}

int
print_insn_i386_att (bfd_vma pc, disassemble_info *info)
{
	return 0;
}

#endif
