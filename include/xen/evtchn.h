#ifdef CONFIG_PARAVIRT_XEN
#include <xen/public/evtchn.h>
#else
/******************************************************************************
 * evtchn.h
 * 
 * Communication via Xen event channels.
 * Also definitions for the device that demuxes notifications to userspace.
 * 
 * Copyright (c) 2004-2005, K A Fraser
 * 
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License version 2
 * as published by the Free Software Foundation; or, when distributed
 * separately from the Linux kernel or incorporated into other
 * software packages, subject to the following license:
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this source file (the "Software"), to deal in the Software without
 * restriction, including without limitation the rights to use, copy, modify,
 * merge, publish, distribute, sublicense, and/or sell copies of the Software,
 * and to permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 * 
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
 * IN THE SOFTWARE.
 */

#ifndef __ASM_EVTCHN_H__
#define __ASM_EVTCHN_H__

#include <linux/interrupt.h>
#include <asm/hypervisor.h>
#include <asm/ptrace.h>
#include <asm/sync_bitops.h>
#include <xen/interface/event_channel.h>
#include <linux/smp.h>

/*
 * LOW-LEVEL DEFINITIONS
 */

#ifdef CONFIG_XEN
struct irq_cfg {
	u32 info;
	union {
		int bindcount; /* for dynamic IRQs */
#ifdef CONFIG_X86_IO_APIC
		unsigned int vector; /* for physical IRQs */
#endif
	};
};
struct irq_cfg *alloc_irq_and_cfg_at(unsigned int at, int node);
static inline int evtchn_make_refcounted(unsigned int evtchn) { return 0; }
#endif

/*
 * Dynamically bind an event source to an IRQ-like callback handler.
 * On some platforms this may not be implemented via the Linux IRQ subsystem.
 * The IRQ argument passed to the callback handler is the same as returned
 * from the bind call. It may not correspond to a Linux IRQ number.
 * Returns IRQ or negative errno.
 */
int bind_caller_port_to_irqhandler(
	unsigned int caller_port,
	irq_handler_t handler,
	unsigned long irqflags,
	const char *devname,
	void *dev_id);
int bind_listening_port_to_irqhandler(
	unsigned int remote_domain,
	irq_handler_t handler,
	unsigned long irqflags,
	const char *devname,
	void *dev_id);
int bind_interdomain_evtchn_to_irqhandler(
	unsigned int remote_domain,
	unsigned int remote_port,
	irq_handler_t handler,
	unsigned long irqflags,
	const char *devname,
	void *dev_id);
int bind_virq_to_irqhandler(
	unsigned int virq,
	unsigned int cpu,
	irq_handler_t handler,
	unsigned long irqflags,
	const char *devname,
	void *dev_id);
#if defined(CONFIG_SMP) && defined(CONFIG_XEN) && defined(CONFIG_X86)
int bind_virq_to_irqaction(
	unsigned int virq,
	unsigned int cpu,
	struct irqaction *action);
#else
#define bind_virq_to_irqaction(virq, cpu, action) \
	bind_virq_to_irqhandler(virq, cpu, (action)->handler, \
				(action)->flags | IRQF_NOBALANCING, \
				(action)->name, action)
#endif
#if defined(CONFIG_SMP) && !defined(MODULE)
#ifndef CONFIG_X86
int bind_ipi_to_irqhandler(
	unsigned int ipi,
	unsigned int cpu,
	irq_handler_t handler,
	unsigned long irqflags,
	const char *devname,
	void *dev_id);
#else
int bind_ipi_to_irqaction(
	unsigned int cpu,
	struct irqaction *action);
DECLARE_PER_CPU(DECLARE_BITMAP(, NR_IPIS), ipi_pending);
#endif
#endif

/*
 * Common unbind function for all event sources. Takes IRQ to unbind from.
 * Automatically closes the underlying event channel (except for bindings
 * made with bind_caller_port_to_irqhandler()).
 */
void unbind_from_irqhandler(unsigned int irq, void *dev_id);

#if defined(CONFIG_SMP) && defined(CONFIG_XEN) && defined(CONFIG_X86)
/* Specialized unbind function for per-CPU IRQs. */
void unbind_from_per_cpu_irq(unsigned int irq, unsigned int cpu,
			     struct irqaction *);
#else
#define unbind_from_per_cpu_irq(irq, cpu, action) \
	unbind_from_irqhandler(irq, action)
#endif

#ifndef CONFIG_XEN
void irq_resume(void);
#endif

/* Entry point for notifications into Linux subsystems. */
asmlinkage
#ifdef CONFIG_PREEMPT
void
#else
bool
#endif
evtchn_do_upcall(struct pt_regs *regs);

/* Mark a PIRQ as unavailable for dynamic allocation. */
void evtchn_register_pirq(int irq);
/* Map a Xen-supplied PIRQ to a dynamically allocated one. */
int evtchn_map_pirq(int irq, unsigned int xen_pirq, unsigned int nr);
/* Look up a Xen-supplied PIRQ for a dynamically allocated one. */
int evtchn_get_xen_pirq(int irq);

void mask_evtchn(int port);
void disable_all_local_evtchn(void);
void unmask_evtchn(int port);
unsigned int irq_from_evtchn(unsigned int port);

static inline int test_and_set_evtchn_mask(int port)
{
	shared_info_t *s = HYPERVISOR_shared_info;
	return sync_test_and_set_bit(port, s->evtchn_mask);
}

static inline void clear_evtchn(int port)
{
	shared_info_t *s = HYPERVISOR_shared_info;
	sync_clear_bit(port, s->evtchn_pending);
}

static inline void set_evtchn(int port)
{
	shared_info_t *s = HYPERVISOR_shared_info;
	sync_set_bit(port, s->evtchn_pending);
}

static inline int test_evtchn(int port)
{
	shared_info_t *s = HYPERVISOR_shared_info;
	return sync_test_bit(port, s->evtchn_pending);
}

static inline void notify_remote_via_evtchn(int port)
{
	struct evtchn_send send = { .port = port };
	VOID(HYPERVISOR_event_channel_op(EVTCHNOP_send, &send));
}

static inline void
multi_notify_remote_via_evtchn(multicall_entry_t *mcl, int port)
{
	struct evtchn_send *send = (void *)(mcl->args + 2);

	BUILD_BUG_ON(sizeof(*send) > sizeof(mcl->args) - 2 * sizeof(*mcl->args));
	send->port = port;
	mcl->op = __HYPERVISOR_event_channel_op;
	mcl->args[0] = EVTCHNOP_send;
	mcl->args[1] = (unsigned long)send;
}

static inline int close_evtchn(int port)
{
	struct evtchn_close close = { .port = port };
	return HYPERVISOR_event_channel_op(EVTCHNOP_close, &close);
}

/* Test an irq's pending state. */
int xen_test_irq_pending(int irq);

/*
 * Use these to access the event channel underlying the IRQ handle returned
 * by bind_*_to_irqhandler().
 */
void notify_remote_via_irq(int irq);
int multi_notify_remote_via_irq(multicall_entry_t *, int irq);
int irq_to_evtchn_port(int irq);

#if defined(CONFIG_SMP) && !defined(MODULE) && defined(CONFIG_X86)
void notify_remote_via_ipi(unsigned int ipi, unsigned int cpu);
void clear_ipi_evtchn(void);
#endif

DECLARE_PER_CPU(bool, privcmd_hcall);

#if defined(CONFIG_XEN_SPINLOCK_ACQUIRE_NESTING) \
    && CONFIG_XEN_SPINLOCK_ACQUIRE_NESTING
void xen_spin_irq_enter(void);
void xen_spin_irq_exit(void);
#else
static inline void xen_spin_irq_enter(void) {}
static inline void xen_spin_irq_exit(void) {}
#endif

#endif /* __ASM_EVTCHN_H__ */
#endif /* CONFIG_PARAVIRT_XEN */
