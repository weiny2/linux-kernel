#include <linux/kernel.h>
#include <linux/unistd.h>
#include <linux/init.h>
#include <linux/kthread.h>
#include <linux/slab.h>
#include <linux/reboot.h>
#include <linux/sched.h>
#include <linux/syscalls.h>
#include <linux/sysrq.h>
#include <asm/hypervisor.h>
#include <xen/xenbus.h>
#include <xen/evtchn.h>
#include <linux/kmod.h>
#include <linux/slab.h>
#include <linux/workqueue.h>

#ifdef HAVE_XEN_PLATFORM_COMPAT_H
#include <xen/platform-compat.h>
#undef handle_sysrq
#endif

#define SHUTDOWN_INVALID  -1
#define SHUTDOWN_POWEROFF  0
#define SHUTDOWN_SUSPEND   2
#define SHUTDOWN_RESUMING  3
#define SHUTDOWN_HALT      4

/* Ignore multiple shutdown requests. */
static int shutting_down = SHUTDOWN_INVALID;

/* Can we leave APs online when we suspend? */
static int fast_suspend;

static void __shutdown_handler(struct work_struct *unused);
static DECLARE_DELAYED_WORK(shutdown_work, __shutdown_handler);

int __xen_suspend(int fast_suspend, void (*resume_notifier)(int));

static int shutdown_process(void *__unused)
{
	static char *envp[] = { "HOME=/", "TERM=linux",
				"PATH=/sbin:/usr/sbin:/bin:/usr/bin", NULL };
	static char *poweroff_argv[] = { "/sbin/poweroff", NULL };

	if ((shutting_down == SHUTDOWN_POWEROFF) ||
	    (shutting_down == SHUTDOWN_HALT)) {
		if (call_usermodehelper("/sbin/poweroff", poweroff_argv,
					envp, 0) < 0) {
#ifdef CONFIG_XEN
			sys_reboot(LINUX_REBOOT_MAGIC1,
				   LINUX_REBOOT_MAGIC2,
				   LINUX_REBOOT_CMD_POWER_OFF,
				   NULL);
#endif /* CONFIG_XEN */
		}
	}

	shutting_down = SHUTDOWN_INVALID; /* could try again */

	return 0;
}

#ifdef CONFIG_PM_SLEEP

static int setup_suspend_evtchn(void);

/* Was last suspend request cancelled? */
static int suspend_cancelled;

static void xen_resume_notifier(int _suspend_cancelled)
{
	int old_state = xchg(&shutting_down, SHUTDOWN_RESUMING);
	BUG_ON(old_state != SHUTDOWN_SUSPEND);
	suspend_cancelled = _suspend_cancelled;
}

static int xen_suspend(void *__unused)
{
	int err, old_state;

	err = set_cpus_allowed_ptr(current, cpumask_of(0));
	if (err) {
		pr_err("Xen suspend can't run on CPU0 (%d)\n", err);
		goto fail;
	}

	do {
		err = __xen_suspend(fast_suspend, xen_resume_notifier);
		if (err) {
			pr_err("Xen suspend failed (%d)\n", err);
			goto fail;
		}
		if (!suspend_cancelled)
			setup_suspend_evtchn();
		old_state = cmpxchg(
			&shutting_down, SHUTDOWN_RESUMING, SHUTDOWN_INVALID);
	} while (old_state == SHUTDOWN_SUSPEND);

	switch (old_state) {
	case SHUTDOWN_INVALID:
	case SHUTDOWN_SUSPEND:
		BUG();
	case SHUTDOWN_RESUMING:
		break;
	default:
		schedule_delayed_work(&shutdown_work, 0);
		break;
	}

	return 0;

 fail:
	old_state = xchg(&shutting_down, SHUTDOWN_INVALID);
	BUG_ON(old_state != SHUTDOWN_SUSPEND);
	return 0;
}

#endif

static void switch_shutdown_state(int new_state)
{
	int prev_state, old_state = SHUTDOWN_INVALID;

	/* We only drive shutdown_state into an active state. */
	if (new_state == SHUTDOWN_INVALID)
		return;

	do {
		/* We drop this transition if already in an active state. */
		if ((old_state != SHUTDOWN_INVALID) &&
		    (old_state != SHUTDOWN_RESUMING))
			return;
		/* Attempt to transition. */
		prev_state = old_state;
		old_state = cmpxchg(&shutting_down, old_state, new_state);
	} while (old_state != prev_state);

	/* Either we kick off the work, or we leave it to xen_suspend(). */
	if (old_state == SHUTDOWN_INVALID)
		schedule_delayed_work(&shutdown_work, 0);
	else
		BUG_ON(old_state != SHUTDOWN_RESUMING);
}

static void __shutdown_handler(struct work_struct *unused)
{
	struct task_struct *taskp;

#ifdef CONFIG_PM_SLEEP
	if (shutting_down == SHUTDOWN_SUSPEND)
		taskp = kthread_run(xen_suspend, NULL, "suspend");
	else
#endif
		taskp = kthread_run(shutdown_process, NULL, "shutdown");

	if (IS_ERR(taskp)) {
		pr_warning("Error creating shutdown process (%ld): "
			   "retrying...\n", -PTR_ERR(taskp));
		schedule_delayed_work(&shutdown_work, HZ/2);
	}
}

static void shutdown_handler(struct xenbus_watch *watch,
			     const char **vec, unsigned int len)
{
	extern void ctrl_alt_del(void);
	char *str;
	struct xenbus_transaction xbt;
	int err, new_state = SHUTDOWN_INVALID;

	if ((shutting_down != SHUTDOWN_INVALID) &&
	    (shutting_down != SHUTDOWN_RESUMING))
		return;

 again:
	err = xenbus_transaction_start(&xbt);
	if (err)
		return;

	str = (char *)xenbus_read(xbt, "control", "shutdown", NULL);
	/* Ignore read errors and empty reads. */
	if (XENBUS_IS_ERR_READ(str)) {
		xenbus_transaction_end(xbt, 1);
		return;
	}

	xenbus_write(xbt, "control", "shutdown", "");

	err = xenbus_transaction_end(xbt, 0);
	if (err == -EAGAIN) {
		kfree(str);
		goto again;
	}

	if (strcmp(str, "poweroff") == 0)
		new_state = SHUTDOWN_POWEROFF;
	else if (strcmp(str, "reboot") == 0)
		ctrl_alt_del();
#ifdef CONFIG_PM_SLEEP
	else if (strcmp(str, "suspend") == 0)
		new_state = SHUTDOWN_SUSPEND;
#endif
	else if (strcmp(str, "halt") == 0)
		new_state = SHUTDOWN_HALT;
	else
		pr_warning("Ignoring shutdown request: %s\n", str);

	switch_shutdown_state(new_state);

	kfree(str);
}

static void sysrq_handler(struct xenbus_watch *watch, const char **vec,
			  unsigned int len)
{
	char sysrq_key = '\0';
	struct xenbus_transaction xbt;
	int err;

 again:
	err = xenbus_transaction_start(&xbt);
	if (err)
		return;
	if (xenbus_scanf(xbt, "control", "sysrq", "%c", &sysrq_key) <= 0) {
		pr_err("Unable to read sysrq code in control/sysrq\n");
		xenbus_transaction_end(xbt, 1);
		return;
	}

	if (sysrq_key != '\0')
		xenbus_printf(xbt, "control", "sysrq", "%c", '\0');

	err = xenbus_transaction_end(xbt, 0);
	if (err == -EAGAIN)
		goto again;

#ifdef CONFIG_MAGIC_SYSRQ
	if (sysrq_key != '\0')
		handle_sysrq(sysrq_key);
#endif
}

static struct xenbus_watch shutdown_watch = {
	.node = "control/shutdown",
	.callback = shutdown_handler
};

static struct xenbus_watch sysrq_watch = {
	.node = "control/sysrq",
	.callback = sysrq_handler
};

#ifdef CONFIG_PM_SLEEP
static irqreturn_t suspend_int(int irq, void* dev_id)
{
	switch_shutdown_state(SHUTDOWN_SUSPEND);
	return IRQ_HANDLED;
}

static int setup_suspend_evtchn(void)
{
	static int irq;
	int port;
	char portstr[16];

	if (irq > 0)
		unbind_from_irqhandler(irq, NULL);

	irq = bind_listening_port_to_irqhandler(0, suspend_int, 0, "suspend",
						NULL);
	if (irq <= 0)
		return -1;

	port = irq_to_evtchn_port(irq);
	pr_info("suspend: event channel %d\n", port);
	sprintf(portstr, "%d", port);
	xenbus_write(XBT_NIL, "device/suspend", "event-channel", portstr);

	return 0;
}
#else
#define setup_suspend_evtchn() 0
#endif

static int setup_shutdown_watcher(void)
{
	int err;

	err = register_xenbus_watch(&sysrq_watch);
	if (err) {
		pr_err("Failed to set sysrq watcher\n");
		return err;
	}

	if (is_initial_xendomain())
		return 0;

	xenbus_scanf(XBT_NIL, "control",
		     "platform-feature-multiprocessor-suspend",
		     "%d", &fast_suspend);

	err = register_xenbus_watch(&shutdown_watch);
	if (err) {
		pr_err("Failed to set shutdown watcher\n");
		return err;
	}

	/* suspend event channel */
	err = setup_suspend_evtchn();
	if (err) {
		pr_err("Failed to register suspend event channel\n");
		return err;
	}

	return 0;
}

#ifdef CONFIG_XEN

static int shutdown_event(struct notifier_block *notifier,
			  unsigned long event,
			  void *data)
{
	setup_shutdown_watcher();
	return NOTIFY_DONE;
}

static int __init setup_shutdown_event(void)
{
	static struct notifier_block xenstore_notifier = {
		.notifier_call = shutdown_event
	};
	register_xenstore_notifier(&xenstore_notifier);

	return 0;
}

subsys_initcall(setup_shutdown_event);

#else /* !defined(CONFIG_XEN) */

int xen_reboot_init(void)
{
	return setup_shutdown_watcher();
}

#endif /* !defined(CONFIG_XEN) */
