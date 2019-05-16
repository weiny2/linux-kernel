#ifndef __ASM_SVOS_H
#define __ASM_SVOS_H

#include <linux/svos.h>

/*
 * defined hook routines used by svos to
 * do work in the arch directory without major impacts
 * to the kernel source.
 */

unsigned long svos_adjgap( unsigned long );
void svos_parse_mem( char * );
void svos_mem_init(void);


#endif
