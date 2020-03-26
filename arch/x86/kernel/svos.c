/*
 * starting point for svos hooks in the arch tree.
 * strategy is to minimize code placement in base kernel
 * files to just hook calls or minor changes.
 */
#include <linux/kernel.h>
#include <linux/types.h>
#include <linux/init.h>
#include <linux/crash_dump.h>
#include <linux/export.h>
#include <linux/mmzone.h>
#include <linux/pfn.h>
#include <linux/suspend.h>
#include <linux/acpi.h>
#include <linux/firmware-map.h>
#include <linux/memblock.h>
#include <linux/sort.h>
#include <linux/svos.h>
#include <asm/e820/api.h>
#include <asm/e820/types.h>
#include <asm/proto.h>
#include <asm/setup.h>
#include <asm/tlbflush.h>
#include <linux/pfn.h>
#include <linux/pci.h>

struct mm_struct *svfs_init_mmP = &init_mm;
EXPORT_SYMBOL(svfs_init_mmP);
EXPORT_SYMBOL(init_mm);

extern struct task_struct *find_task_by_pid_ns(pid_t nr, struct pid_namespace *ns);
EXPORT_SYMBOL(find_task_by_pid_ns);

void svos_flush_tlb_page(struct vm_area_struct *vma, unsigned long page_addr)
{
	flush_tlb_page(vma, page_addr);
}
EXPORT_SYMBOL(svos_flush_tlb_page);

extern struct list_head pci_mmcfg_list;
struct list_head *svfs_pci_mmcfg_listP = &pci_mmcfg_list;
EXPORT_SYMBOL(svfs_pci_mmcfg_listP);
EXPORT_SYMBOL(pci_mmcfg_list);

extern unsigned long mmu_cr4_features;

unsigned long    svos1stTargetPage;
EXPORT_SYMBOL(svos1stTargetPage);
unsigned long *svos_mmu_cr4_featuresP=&mmu_cr4_features;
EXPORT_SYMBOL(svos_mmu_cr4_featuresP);

unsigned long svos_max_pfn;
EXPORT_SYMBOL(svos_max_pfn);
int svos_memory_split = 0;
unsigned long long    svos_split_after = 0x4000000; //Default is 64MB
unsigned long long    svos_split_above = 0x100000000;
EXPORT_SYMBOL(svos_split_after);
EXPORT_SYMBOL(svos_split_above);

int     hole_reserved = 0;
struct e820_table e820_svos;
EXPORT_SYMBOL(e820_svos);

struct	svos_node_memory svos_node_memory[MAX_NUMNODES];
nodemask_t svos_nodes_parsed;
EXPORT_SYMBOL(svos_node_memory);
EXPORT_SYMBOL(svos_nodes_parsed);

extern long ksys_mmap_pgoff(unsigned long , unsigned long , unsigned long , unsigned long , unsigned long , unsigned  long );
EXPORT_SYMBOL(ksys_mmap_pgoff);

extern int nr_ioapics;
int *svos_nr_ioapicsP = &nr_ioapics;
EXPORT_SYMBOL(svos_nr_ioapicsP);

extern int mp_bus_id_to_type[MAX_MP_BUSSES];
EXPORT_SYMBOL(mp_bus_id_to_type);

typedef struct irq_desc* vector_irq_t[NR_VECTORS];
DECLARE_PER_CPU(vector_irq_t, vector_irq);
EXPORT_SYMBOL(vector_irq);

__init unsigned long
svos_adjgap( unsigned long gapsize )
{
	unsigned long round;
	unsigned long start;

	round = 0x100000;
	while ((gapsize >> 4) > round)
		round += round;

	start = (gapsize + round) & -round;
	return start;
}

/*
 * Handle the svos memory parameters
 */
static __init int memory_setup(char *opt)
{ 
	if (!opt)
		return -EINVAL;

	if(!strncmp(opt, "split_above=", 12)){
		opt+=12;
		svos_split_above = memparse(opt, &opt);
	}

	if(!strncmp(opt, "split=", 6)){
		opt+=6;
		svos_memory_split = memparse(opt, &opt);
	}
	if(!strncmp(opt, "split_after=", 12)) {
		opt+=12;
		svos_split_after = memparse(opt, &opt);
	}
	
	return 0;
} 
early_param("svos_memory", memory_setup);
	
#define GAP_SIZE	0x40000000LL // 1GB 
void __init svos_mem_init(void)
{
	unsigned long long target_space = svos1stTargetPage << PAGE_SHIFT;
	unsigned long long accum_size;
	unsigned long long above_addr;
	struct e820_entry *ep;
	int i;

	svos_max_pfn = e820__end_of_ram_pfn();
	memset(&e820_svos,0,sizeof(e820_svos));
	memcpy(&e820_svos,e820_table,sizeof(struct e820_table));
	//
	// if no svos memory is specified or svos@ is zero
	// act as if svos@ is set to max mem.
	//
	if( target_space == 0ULL )
		return;

	//
	// scan the e820 map and figure out what address will give
	// us enough memory to satisfy the svos@ boot parameter.
	// scan only the high memory > 4G.
	// we shouldn't forget the take into account split_after and split_above.
	//
	// Simple case no split just look for an addr that wil give
	// us the requested kernel memory and remove the rest for SVOS.
	//
	accum_size = 0;
	if( svos_memory_split == 0 ){
		for (i = 0; i < e820_table->nr_entries; i++) {
			ep =  &e820_table->entries[i];
			if( ep->type != E820_TYPE_RAM )
				continue;

			if( (accum_size + ep->size) >= target_space ){
				e820__range_remove( ep->addr + (target_space - accum_size), ULLONG_MAX, E820_TYPE_RAM,1);
				return;
			}

			accum_size += ep->size;
		}
		// fall through the else to common error return
	}

	//
	// more complicated case. 
	// Take into account split_after and split_above values.
	// Kernel gets memory from 0 - split_after.
	// SVOS   gets memory from split_after to split_above.
	// Kernel gets memory from split_above to (SVOS@ - split_after).
	// SVOS   get the remainder.
	// the effective value of split_above may need to be modified 
	// to take into account holes in ram especially in high ram
	// where the holes can be GBs in size.
	//

	else {
		above_addr = 0;
		for(i = 0; i < e820_table->nr_entries; i++){
			ep = &e820_table->entries[i];
			if( ep->type != E820_TYPE_RAM )
				continue;

			if( (ep->addr + ep->size) <= svos_split_above )
				continue; 		

			if( above_addr == 0 ){		// first segment to look at
				accum_size = svos_split_after;
				//
				// set the above address to be either the boot param value
				// or the first memory address above the boot param value.
				//
				if( ep->addr < svos_split_above ) {
					above_addr = svos_split_above;
					accum_size += ep->size - (svos_split_above - ep->addr);
				}
				else{
					above_addr = ep->addr;
					accum_size += ep->size;
				}
			}
			else
				accum_size += ep->size;
			//
			// Once we have accumulated enough space
			// remove the svos areas from the e820 map.
			//
			if( accum_size >= target_space ){
				e820__range_remove( svos_split_after, (svos_split_above - svos_split_after), E820_TYPE_RAM,1);
				e820__range_remove( above_addr + (target_space - svos_split_after), ULLONG_MAX, E820_TYPE_RAM,1);
				return;
			}
				
		}
	}
	//
	// common error return
	//
	printk( KERN_ERR "%s : not enough memory to satisfy svos@ mem parameter\n", __FUNCTION__);
	return;
}

/*
 * called from the parse_memopt handling to initialize the svosmem 
 * parameter.
 */
void
svos_parse_mem( char *p )
{
	u64	mem_size = memparse( p+5, &p );

	svos1stTargetPage = mem_size >>PAGE_SHIFT;
	return;
}

//
// Trap hook called from do_trap_no_signal on traps
//   the handler return code tells trap code whether to
//   continue the normal processing or return.
//
int (*svTrapHandlerKernelP)(int index, struct pt_regs *regs);
EXPORT_SYMBOL(svTrapHandlerKernelP);
int
svos_trap_hook( int trapnr, struct pt_regs *regs)
{
	int	trapResult = 0;
	if( svTrapHandlerKernelP != NULL) {
		trapResult = svTrapHandlerKernelP(trapnr, regs);
	}
	return trapResult;
}
EXPORT_SYMBOL(svos_trap_hook);

//
// hooks for vtd interrupt managment interface.
// this hooks are for the dmar driver.
// 
//
int (*svfs_vtdSubmitSync)(u64, void *) = NULL;
EXPORT_SYMBOL(svfs_vtdSubmitSync);
int (*svos_vtdFaultHandler)(u64, void *) = NULL;
EXPORT_SYMBOL(svos_vtdFaultHandler);
void svfs_ResetInfo(u64 reg_phys_address)
{
	printk(KERN_CRIT "%s called to handle address - %LX\n", __FUNCTION__,
		reg_phys_address);
}
EXPORT_SYMBOL(svfs_ResetInfo);

void
svfs_lock_pci(void)
{
//	spin_lock_irq(&pci_lock);
	return;
}
void
svfs_unlock_pci(void)
{
//	spin_unlock_irq(&pci_lock);
	return;
}
void
svfs_lock_pci_irqsave(unsigned long *flags)
{
//	spin_lock_irq(&pci_lock);
	return;
}
void
svfs_unlock_pci_irqrestore(unsigned long *flags)
{
//	spin_unlock_irq(&pci_lock);
	return;
}
EXPORT_SYMBOL(svfs_lock_pci);
EXPORT_SYMBOL(svfs_unlock_pci);
EXPORT_SYMBOL(svfs_lock_pci_irqsave);
EXPORT_SYMBOL(svfs_unlock_pci_irqrestore);

extern int pci_setup_device( struct pci_dev *);
int
svfs_pci_setup_device( struct pci_dev *dev )
{
	return pci_setup_device(dev);
}
EXPORT_SYMBOL(svfs_pci_setup_device);
extern void pci_device_add( struct pci_dev *, struct pci_bus *);
void
svfs_pci_device_add( struct pci_dev *dev, struct pci_bus *bus)
{
	pci_device_add( dev, bus );
}
EXPORT_SYMBOL(svfs_pci_device_add);


unsigned long svfs_get_cr4_features(void);
void svfs_set_in_cr4(unsigned long mask);
void svfs_clear_in_cr4(unsigned long mask);


unsigned long
svfs_get_cr4_features(void)
{
	return mmu_cr4_features;
}

void
svfs_set_in_cr4(unsigned long mask)
{
	cr4_set_bits(mask);
}

void
svfs_clear_in_cr4(unsigned long mask)
{
	cr4_clear_bits(mask);
}
EXPORT_SYMBOL(svfs_get_cr4_features);
EXPORT_SYMBOL(svfs_set_in_cr4);
EXPORT_SYMBOL(svfs_clear_in_cr4);

// For SVFS interrupt support
extern int __irq_domain_alloc_irqs(struct irq_domain *domain, int irq_base,
			    unsigned int nr_irqs, int node, void *arg,
			    bool realloc, const struct irq_affinity_desc *affinity);
EXPORT_SYMBOL(__irq_domain_alloc_irqs);

// For testbox
extern u32 *trampoline_cr4_features;
EXPORT_SYMBOL(trampoline_cr4_features);
extern void flush_tlb_all(void);
EXPORT_SYMBOL(flush_tlb_all);
extern void set_cpu_online(unsigned int cpu, bool online);
EXPORT_SYMBOL(set_cpu_online);
