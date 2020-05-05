// SPDX-License-Identifier: GPL-2.0+
#define pr_fmt(fmt)    "uintr: " fmt

#include <linux/sched.h>
#include <linux/slab.h>
#include <linux/uaccess.h>
#include <linux/uintr.h>

#include <asm/msr.h>
#include <asm/msr-index.h>

union uintr_upid_status_control {
	struct {
		u64 on:1;
		u64 sn:1;
		u64 reserved1:14;
		u64 nv:8;
		u64 reserved2:8;
		u64 ndst:32;
	};
	u64 raw;
} __packed;

struct uintr_upid_struct  {
	union uintr_upid_status_control sc;
	u64 puir;
} __aligned(64);

/* User IPI Target Table Entry (UITTE) */
struct uintr_uitte {
	u64	valid:1,
		reserved1:7,
		user_vec:6,
		user_vec_zero:2,
		reserved2:48;
	u64	target_upid_addr;
} __packed __aligned(64);

struct uintr_receiver {
	u64 handler;
	struct uintr_upid_struct *upid_uaddr;
	struct uintr_upid_struct *upid_kaddr;
	u64 uirr; // Not needed with xsave
	u64 uvec_mask; /* track active uvec per vector per bit */
};

struct uintr_sender {
	struct uintr_uitte *uitt_uaddr;
	struct uintr_uitte *uitt_kaddr;
	u64 uitt_mask; /* track active uitt per vector per bit */
};

// Why is this needed?
#define UINTR_REDZONE 128
#define UINTR_NV_MASK 0xffULL

#define UINTR_MAX_UITT_NR 64
#define UINTR_MAX_UVEC_NR 64

static struct uintr_upid_struct dummy_upid = {
	.sc = {
		.on = 1,
		.sn = 1,
		.reserved1 = 0,
		.nv = 0,
		.reserved2 = 0,
		.ndst = 0,
	},
	.puir = 0,
};

static inline bool is_uvec_active(struct task_struct *t)
{
	return !!t->thread.ui_recv;
}

static inline bool is_uipi_active(struct task_struct *t)
{
	return !!t->thread.ui_send;
}

static inline bool uintr_is_pending(struct task_struct *t)
{
	return(t->thread.ui_recv->upid_kaddr->puir != 0);
}

/* If task has registered UPID or active UITT */
static inline bool is_uintr_task(struct task_struct *t)
{
	return(is_uvec_active(t) || is_uipi_active(t));
}

static inline u32 cpu_to_ndst(int cpu)
{
	u32 apicid = (u32)apic->cpu_present_to_apicid(cpu);

	if (apicid == BAD_APICID)
		pr_err("Bad apic id\n");

	if (!x2apic_enabled())
		return (apicid << 8) & 0xFF00;

	return apicid;
}

/* Need to check if feature present but disabled */
inline bool uintr_arch_enabled(void)
{
	return static_cpu_has(X86_FEATURE_UINTR);
}

/* Can be further optimized by reducing the number of MSR writes */
inline void uintr_switch(struct task_struct *prev, struct task_struct *next)
{
	u64 misc_msr = 0;

	if (!uintr_arch_enabled())
		return;

	if (is_uvec_active(prev)) {
		/* stop sending UINTR interrutps to this thread */
		prev->thread.ui_recv->upid_kaddr->sc.sn = 1;

		wrmsrl_safe(MSR_IA32_UINT_MISC, 0ULL);
		rdmsrl_safe(MSR_IA32_UINT_RR, &prev->thread.ui_recv->uirr);
		wrmsrl_safe(MSR_IA32_UINT_RR, 0ULL);
		wrmsrl_safe(MSR_IA32_UINT_PD, 0ULL);
		wrmsrl_safe(MSR_IA32_UINT_HANDLER, 0ULL);
		trace_printk("save and clear UPID for task %s\n", prev->comm);
	}

	if (is_uipi_active(prev)) {
		wrmsrl_safe(MSR_IA32_UINT_TT, 0ULL);
		wrmsrl_safe(MSR_IA32_UINT_MISC, 0ULL);
		trace_printk("clear UITT for task %s\n", prev->comm);
	}

	if (is_uipi_active(next)) {
		wrmsrl_safe(MSR_IA32_UINT_TT, ((u64)next->thread.ui_send->uitt_uaddr)
						| 1);
		misc_msr |= UINTR_MAX_UITT_NR;
		wrmsrl_safe(MSR_IA32_UINT_MISC, misc_msr);
		trace_printk("restore UITT for task %s\n", next->comm);
	}

	if (is_uvec_active(next)) {
		next->thread.ui_recv->upid_kaddr->sc.ndst =
				cpu_to_ndst(smp_processor_id());

		wrmsrl_safe(MSR_IA32_UINT_PD, (u64)next->thread.ui_recv->upid_uaddr);
		wrmsrl_safe(MSR_IA32_UINT_HANDLER,
			    (u64)next->thread.ui_recv->handler);
		wrmsrl_safe(MSR_IA32_UINT_RR, (u64)next->thread.ui_recv->uirr);

		misc_msr |= ((u64)UINTR_NOTIFICATION_VECTOR) << 32;
		wrmsrl_safe(MSR_IA32_UINT_MISC, misc_msr);

		trace_printk("restore UPID for task %s\n", next->comm);
		next->thread.ui_recv->upid_kaddr->sc.sn = 0;

		/* Interrupts might have been accumulated in the UPID while the
		 * thread was preempted. if this is the case then invoke the
		 * ucode detection sequence manually by sending a self IPI.
		 * Since the notification vector is set and SN is cleared, any
		 * new notifications would result in the ucode updating the UIRR
		 * directly.
		 */
		if (uintr_is_pending(next))
			apic->send_IPI_self(UINTR_NOTIFICATION_VECTOR);
	}
}

static void dump_uitt(struct uintr_uitte *uitt, int nr)
{
	int i;
	u64 *buf = (u64 *)uitt;

	if (nr > UINTR_MAX_UITT_NR)
		return;
	pr_debug("debug: UITT for task=%d entries=%d at address=%llx\n",
		 current->pid, nr, (u64)uitt);
	/* 2 u64 in each UITTE */
	for (i = 0; i < (nr << 1); i = i+2) {
		if ((buf[i] == 0) && (buf[i+1] == 0))
			continue;
		pr_debug("%d: %llx %llx\n", i/2, buf[i], buf[i+1]);
	}
}

/* Replace with self managed memory allocater */
static int uintr_alloc_uitt(struct task_struct *task, unsigned int nr_entries)
{
	//void *addr = kzalloc(sizeof(struct uintr_uitte) * nr_entries, GFP_KERNEL);
	void *addr = kzalloc(PAGE_SIZE, GFP_KERNEL);
	void *va = NULL;

	if(!addr)
		return -ENOSPC;

	va = addr;
	task->thread.ui_send->uitt_kaddr = addr;
	task->thread.ui_send->uitt_uaddr = va;
	return 0;
}

static void uintr_free_uitt(struct uintr_sender *ui_send, struct task_struct *t)
{
	kfree(ui_send->uitt_kaddr);
	ui_send->uitt_uaddr = NULL;
	ui_send->uitt_kaddr = NULL;
}

void uintr_notify_receiver_exit(struct task_struct *task, unsigned int entry)
{
	task->thread.ui_send->uitt_kaddr[entry].target_upid_addr = (u64)&dummy_upid;
}

void uintr_unregister_sender(struct task_struct *t, unsigned int entry)
{
	u64 msr64;

	if (entry >= UINTR_MAX_UITT_NR)
		return;

	if (!is_uipi_active(t))
		return;

	memset(&t->thread.ui_send->uitt_kaddr[entry], 0, sizeof(struct uintr_uitte));
	clear_bit(entry, (unsigned long *)&t->thread.ui_send->uitt_mask);

	if (!t->thread.ui_send->uitt_mask) {
		uintr_free_uitt(t->thread.ui_send, t);
		kfree(t->thread.ui_send);
		t->thread.ui_send = NULL;

		if (t == current) {
			rdmsrl_safe(MSR_IA32_UINT_MISC, &msr64);
			msr64 &= ~0xFFFFFFFF;
			wrmsrl_safe(MSR_IA32_UINT_MISC, msr64);
			wrmsrl_safe(MSR_IA32_UINT_TT, 0ULL);
		}
	}
}

static int uintr_sender_init(struct task_struct *t)
{
	u64 msr64;

	t->thread.ui_send = kzalloc(sizeof(*t->thread.ui_send), GFP_KERNEL);
	if (!t->thread.ui_send)
		return -ENOSPC;

	if (uintr_alloc_uitt(t, UINTR_MAX_UITT_NR)) {
		pr_debug("send: alloc uitt failed for task=%d\n", t->pid);
		kfree(t->thread.ui_send);
		t->thread.ui_send = NULL;
		return -ENOSPC;
	}

	rdmsrl_safe(MSR_IA32_UINT_TT, &msr64);
	if (!msr64) {
		wrmsrl_safe(MSR_IA32_UINT_TT, ((u64)t->thread.ui_send->uitt_uaddr) |
						1);
		pr_debug("send: set UITT address for task=%d\n", t->pid);
	} else {
		pr_debug("send: ERROR: UITT for task=%d already set as %llx\n\n",
			 t->pid, (u64)msr64);
		return -EINVAL;
	}

	rdmsrl_safe(MSR_IA32_UINT_MISC, &msr64);
	pr_debug("send: old UINT_MISC MSR=%llx\n", msr64);
	msr64 &= ~0xFFFFFFFF;
	msr64 |= UINTR_MAX_UITT_NR;
	wrmsrl_safe(MSR_IA32_UINT_MISC, msr64);
	pr_debug("send: new UINT_MISC MSR=%llx\n", msr64);

	return 0;

}

int uintr_register_sender(struct task_struct *recv_task, int uvec_no)
{

	struct uintr_uitte *uitte = NULL;
	struct uintr_sender *ui_send;
	int ret = 0;
	int entry;

	/* Allocate an entry in UITT for the sender to link to a
	 * receiver task
	 */

	if (!current->thread.ui_send) {
		ret = uintr_sender_init(current);
		if (ret)
			return ret;
	}

	ui_send = current->thread.ui_send;

	dump_uitt(ui_send->uitt_kaddr, UINTR_MAX_UITT_NR);

	//Assert uitt is not NULL?
	// UITT is always allocated and deallocated with ui_send

	entry = find_first_zero_bit((unsigned long *)&ui_send->uitt_mask,
				    UINTR_MAX_UITT_NR);

	// TODO: Check if no uitt entries left
	ui_send->uitt_mask |= BIT(entry);
	pr_debug("send: use UITT entry %d, mask %llx\n", entry,
		 ui_send->uitt_mask);

	uitte = &ui_send->uitt_kaddr[entry];
	uitte->valid = 1;
	uitte->user_vec = uvec_no;
	uitte->target_upid_addr = (u64)recv_task->thread.ui_recv->upid_uaddr;

	dump_uitt(ui_send->uitt_kaddr, UINTR_MAX_UITT_NR);

	return entry;
}

/* Replace with self managed memory allocater */
static void uintr_free_upid(struct uintr_receiver	*ui_recv, struct task_struct *t)
{
	kfree(ui_recv->upid_kaddr);
	ui_recv->upid_kaddr = NULL;
	ui_recv->upid_uaddr = NULL;
}

static void *uintr_alloc_upid(struct task_struct *task)
{
	//void *addr = kzalloc(sizeof(struct uintr_upid_struct), GFP_KERNEL);
	void *addr = kzalloc(PAGE_SIZE, GFP_KERNEL);
	void *va = NULL;

	if(!addr)
		return -ENOSPC;

	va = addr;
	task->thread.ui_recv->upid_kaddr = addr;
	task->thread.ui_recv->upid_uaddr = va;

	return 0;
}

static int uintr_register_handler(u64 handler)
{
	struct uintr_upid_struct *upid;
	struct uintr_receiver *ui_recv;
	struct task_struct *t = current;
	u64 misc_msr;

	t->thread.ui_recv = kzalloc(sizeof(*t->thread.ui_recv), GFP_KERNEL);
	if (!t->thread.ui_recv)
		return -ENOSPC;

	ui_recv = t->thread.ui_recv;

	ui_recv->handler = handler;

	if (uintr_alloc_upid(t)) {
		pr_debug("recv: alloc upid failed for task=%d\n", t->pid);
		kfree(t->thread.ui_recv);
		t->thread.ui_recv = NULL;
		return -ENOSPC;
	}

	ui_recv->upid_kaddr->sc.raw = 0;
	ui_recv->upid_kaddr->puir = 0;

	upid = ui_recv->upid_kaddr;
	upid->sc.nv = UINTR_NOTIFICATION_VECTOR;
	upid->sc.ndst = cpu_to_ndst(smp_processor_id());

	wrmsrl_safe(MSR_IA32_UINT_HANDLER, (u64)handler);
	wrmsrl_safe(MSR_IA32_UINT_PD, (u64)ui_recv->upid_uaddr);

	/* the notification vector is what the ucode actually looks at,
	 * so it has to be set only after everything else is ready
	 */
	rdmsrl_safe(MSR_IA32_UINT_MISC, &misc_msr);
	misc_msr |= (u64)UINTR_NOTIFICATION_VECTOR << 32;
	wrmsrl_safe(MSR_IA32_UINT_MISC, misc_msr);
	pr_debug("recv: UINT_MISC_MSR=%llx\n", misc_msr);

	pr_debug("recv: task=%d register handler=%llx upid %llx\n",
		 t->pid, (u64)handler, (u64)upid);

	return 0;
}

/* TODO: Add code to send ipi notification */
void uintr_set_uvec(struct task_struct *task, int uvec)
{
	task->thread.ui_recv->upid_kaddr->puir |= uvec;
}

int uintr_alloc_uvec(u64 handler)
{
	struct uintr_receiver *ui_recv;
	int uvec;
	int ret;

	if (!current->thread.ui_recv) {
		ret = uintr_register_handler(handler);
		if (ret)
			return ret;
	}

	ui_recv = current->thread.ui_recv;
	if (ui_recv->handler != handler)
		return -EINVAL;

	uvec = find_first_zero_bit((unsigned long *)&ui_recv->uvec_mask,
				   UINTR_MAX_UVEC_NR);

	// Is this check valid?
	if (uvec >= UINTR_MAX_UVEC_NR)
		return -ENOSPC;

	ui_recv->uvec_mask |= BIT(uvec);
	pr_debug("recv: task %d new uvec=%d, new mask %llx\n",
		 current->pid, uvec, ui_recv->uvec_mask);

	return uvec;
}

void uintr_free_uvec(struct task_struct *t, int uvec)
{
	u64 msr64;

	if (uvec < UINTR_MAX_UVEC_NR)
		clear_bit(uvec, (unsigned long *)&t->thread.ui_recv->uvec_mask);

	if (t->thread.ui_recv->uvec_mask)
		return;

	pr_debug("recv: task=%d unregistered all user vectors\n", t->pid);

	if (t == current) {
		rdmsrl_safe(MSR_IA32_UINT_MISC, &msr64);
		msr64 &= ~0xFF00000000;
		wrmsrl_safe(MSR_IA32_UINT_MISC, msr64);
		wrmsrl_safe(MSR_IA32_UINT_PD, 0ULL);
		wrmsrl_safe(MSR_IA32_UINT_RR, 0ULL);
		wrmsrl_safe(MSR_IA32_UINT_HANDLER, 0ULL);
	}

	// TODO: Verify code for dummy upid
	uintr_free_upid(t->thread.ui_recv, t);
	kfree(t->thread.ui_recv);
	t->thread.ui_recv = NULL;
}

void uintr_free(struct task_struct *t)
{
	if (!is_uintr_task(t))
		return;

	if (t == current) {
		wrmsrl_safe(MSR_IA32_UINT_MISC, 0ULL);
		wrmsrl_safe(MSR_IA32_UINT_TT, 0ULL);
		wrmsrl_safe(MSR_IA32_UINT_PD, 0ULL);
		wrmsrl_safe(MSR_IA32_UINT_RR, 0ULL);
		wrmsrl_safe(MSR_IA32_UINT_HANDLER, 0ULL);
		wrmsrl_safe(MSR_IA32_UINT_STACKADJUST, 0ULL);
	}

	// Free uitt
	if (is_uipi_active(t)) {
		uintr_free_uitt(t->thread.ui_send, t);
		kfree(t->thread.ui_send);
		t->thread.ui_send = NULL;
	}

	// Free upid
	if (is_uvec_active(t)) {
		uintr_free_upid(t->thread.ui_recv, t);
		kfree(t->thread.ui_recv);
		t->thread.ui_recv = NULL;
	}
}

void uintr_print_debug_info(void)
{

	u64 msr64_1, msr64_2, msr64_3, msr64_4, msr64_5;
	u64 sc = 0;
	u64 puir = 0;

	if (current->thread.ui_send)
		dump_uitt(current->thread.ui_send->uitt_kaddr, UINTR_MAX_UITT_NR);

	if (current->thread.ui_recv) {
		sc = current->thread.ui_recv->upid_kaddr->sc.raw;
		puir = current->thread.ui_recv->upid_kaddr->puir;
	}

	rdmsrl_safe(MSR_IA32_UINT_MISC, &msr64_1);
	rdmsrl_safe(MSR_IA32_UINT_RR, &msr64_2);
	rdmsrl_safe(MSR_IA32_UINT_PD, &msr64_3);
	rdmsrl_safe(MSR_IA32_UINT_HANDLER, &msr64_4);
	rdmsrl_safe(MSR_IA32_UINT_TT, &msr64_5);

	pr_debug("debug: task=%d upid.l=%llx puir=%llx uirr=%llx misc=%llx pd=%llx handler=%llx uitt=%llx\n",
			 current->pid, sc, puir,
			 msr64_2, msr64_1, msr64_3, msr64_4, msr64_5);

}

