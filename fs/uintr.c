// SPDX-License-Identifier: GPL-2.0+
#define pr_fmt(fmt)	"uintr: " fmt

#include <linux/anon_inodes.h>
#include <linux/fdtable.h>
#include <linux/sched.h>
#include <linux/syscalls.h>
#include <linux/uintr.h>

#define	UINTR_RECEIVER_DEBUG_INFO	(1 << 10)
#define	UINTR_SENDER_SET_UVEC		(1 << 20)
#define	UINTR_SENDER_DEBUG_INFO		(1 << 21)


struct uintr_receiver_ctx {
	int uvec_no;
	struct task_struct *r_task;
	/* sender_lock to protect the sender_list */
	spinlock_t sender_lock;
	struct list_head sender_list;
};

struct uipi_sender_ctx {
	int uipi_id;
	struct uintr_receiver_ctx *recv_ctx;
	wait_queue_head_t wait_uipifd;
	struct list_head node;
	struct task_struct *s_task;
};

static ssize_t uipifd_read(struct file *file, char __user *buf, size_t count,
			   loff_t *ppos)
{
	int *ret_uipi_id;
	struct uipi_sender_ctx *send_ctx;
	int uipi_id;

	/* Check invalid ppos value */
	if (count < sizeof(*ret_uipi_id))
		return -EINVAL;

	send_ctx = (struct uipi_sender_ctx *)file->private_data;
	// Validate buf
	ret_uipi_id = (int *)buf;
	if (!send_ctx->recv_ctx)
		uipi_id = -1;
	else
		uipi_id = send_ctx->uipi_id;
	if (put_user(uipi_id, ret_uipi_id))
		return -EFAULT;

	return sizeof(*ret_uipi_id);
}

static __poll_t uipifd_poll(struct file *file, poll_table *pt)
{
	struct uipi_sender_ctx *send_ctx = file->private_data;
	int poll_flags = 0;

	//pr_debug("send: task=%d called poll for uipi id=%d\n",
	//	   current->pid, send_ctx->uipi_id);

	poll_wait(file, &send_ctx->wait_uipifd, pt);

	if (!send_ctx->recv_ctx)
		poll_flags = EPOLLHUP | EPOLLERR;

	return poll_flags;
}

static int uipifd_release(struct inode *inode, struct file *file)
{
	struct uipi_sender_ctx  *send_ctx;
	unsigned long flags;

	send_ctx = (struct uipi_sender_ctx *)file->private_data;

	pr_debug("send: releasing uipi id %d for task=%d\n",
		 send_ctx->uipi_id, send_ctx->s_task->pid);

	// TODO: Handle the case when s_task has already exited
	uintr_unregister_sender(send_ctx->s_task, send_ctx->uipi_id);

	// TODO: Verify remove link from receiver FD
	// Is it still possible to have a race?

	if (send_ctx->recv_ctx) {
		spin_lock_irqsave(&send_ctx->recv_ctx->sender_lock, flags);
		list_del(&send_ctx->node);
		spin_unlock_irqrestore(&send_ctx->recv_ctx->sender_lock, flags);
	}

	kfree(send_ctx);

	return 0;
}

#ifdef CONFIG_PROC_FS
static void uipifd_show_fdinfo(struct seq_file *m, struct file *file)
{
	struct uipi_sender_ctx *send_ctx = file->private_data;

	seq_printf(m, "uipi_id:%d\n", send_ctx->uipi_id);
}
#endif

static const struct file_operations uipifd_fops = {
#ifdef CONFIG_PROC_FS
	.show_fdinfo	= uipifd_show_fdinfo,
#endif
	.release	= uipifd_release,
	.poll		= uipifd_poll,
	.read		= uipifd_read,
	.llseek		= noop_llseek,
};

static int do_uipi_send(unsigned int uintrfd, unsigned int flags)
{
	struct uintr_receiver_ctx *recv_ctx;
	struct file *file_recv;
	struct file *file_send;
	int fd;
	struct uipi_sender_ctx *send_ctx;
	unsigned long lock_flags;

	if (!uintr_arch_enabled())
		return -EINVAL;

	if (flags & UINTR_SENDER_DEBUG_INFO) {
		uintr_print_debug_info();
		return 0;
	}

	pr_debug("send: task=%d uintrfd %d flags %d",
			current->pid, uintrfd, flags);

	if (!uintrfd)
		return -EINVAL;

	// handle receiver task being outside this mm
	file_recv = fcheck(uintrfd);

	if (!file_recv)
		return -EINVAL;

	recv_ctx = (struct uintr_receiver_ctx *)file_recv->private_data;

	if (!recv_ctx)
		return -EINVAL;



	if (flags & UINTR_SENDER_SET_UVEC) {
		/* Generating some other vector instead of the registered one */
		// User recv_ctx->uvec_no for registered vector
		uintr_set_uvec(recv_ctx->r_task, 0x10);
		return 0;
	}

	pr_debug("send: recv_task_addr=%llx and recv_task_id=%d fd private data=%llx\n",
			(u64)recv_ctx->r_task,
			recv_ctx->r_task->pid,
			(u64)file_recv->private_data);

	send_ctx = kzalloc(sizeof(*send_ctx), GFP_KERNEL);
	if (!send_ctx)
		return -ENOMEM;

	send_ctx->uipi_id = uintr_register_sender(recv_ctx->r_task,
						  recv_ctx->uvec_no);
	if (send_ctx->uipi_id < 0) {
		kfree(send_ctx);
		return -ENOSPC;
	}
	init_waitqueue_head(&send_ctx->wait_uipifd);

	send_ctx->s_task = current;
	send_ctx->recv_ctx = recv_ctx;

	spin_lock_irqsave(&recv_ctx->sender_lock, lock_flags);

	// TODO: Verify add sender pointer to receiver FD
	list_add(&send_ctx->node, &recv_ctx->sender_list);

	spin_unlock_irqrestore(&recv_ctx->sender_lock, lock_flags);

	pr_debug("send: uipi id index=%d", send_ctx->uipi_id);

	fd = anon_inode_getfd("[uipifd]", &uipifd_fops, NULL,
			      O_RDONLY | O_CLOEXEC);

	file_send = fcheck(fd);
	file_send->private_data = (void *)send_ctx;

	pr_debug("send: registered successfully\n");
	return fd;
}

/*
 * sys_uipi_send - setup user inter-processor interrupt sender.
 */
SYSCALL_DEFINE2(uipi_send, unsigned int, uintrfd, unsigned int, flags)

{
	return do_uipi_send(uintrfd, flags);
}

static ssize_t uintrfd_read(struct file *file, char __user *buf, size_t count,
			   loff_t *ppos)
{
	int *ret_uvec;
	struct uintr_receiver_ctx *recv_ctx;

	recv_ctx = (struct uintr_receiver_ctx *)file->private_data;

	/* handle invalid ppos */
	if (count < sizeof(*ret_uvec))
		return -EINVAL;

	// Validate buf
	ret_uvec = (int *)buf;
	if (put_user(recv_ctx->uvec_no, ret_uvec))
		return -EFAULT;

	return sizeof(*ret_uvec);
}

static int uintrfd_release(struct inode *inode, struct file *file)
{
	struct uintr_receiver_ctx *recv_ctx;
	struct uipi_sender_ctx *send_ctx;
	struct task_struct *task;
	unsigned long flags;

	recv_ctx = (struct uintr_receiver_ctx *)file->private_data;
	task = recv_ctx->r_task;
	pr_debug("recv: releasing uvec=%d for task=%d\n",
		 recv_ctx->uvec_no, task->pid);


	spin_lock_irqsave(&recv_ctx->sender_lock, flags);

	// TODO: Check closing of all the sender uipi fds for this receiver
	list_for_each_entry(send_ctx, &recv_ctx->sender_list, node) {
		pr_debug("recv: waking up sender task=%d uipi id %d\n",
			 send_ctx->s_task->pid, send_ctx->uipi_id);
		send_ctx->recv_ctx = NULL;
		uintr_notify_receiver_exit(send_ctx->s_task, send_ctx->uipi_id);
		wake_up_all(&send_ctx->wait_uipifd);
	}

	spin_unlock_irqrestore(&recv_ctx->sender_lock, flags);

	uintr_free_uvec(task, recv_ctx->uvec_no);

	kfree(recv_ctx);

	return 0;
}

static __poll_t uintrfd_poll(struct file *file, struct poll_table_struct *pts)
{
	struct uintr_receiver_ctx  *recv_ctx = file->private_data;
	struct uipi_sender_ctx *send_ctx;
	int poll_flags = 0;
	unsigned long flags;


	spin_lock_irqsave(&recv_ctx->sender_lock, flags);

	pr_debug("recv: task=%d called poll for uvec no=%d\n",
		 current->pid, recv_ctx->uvec_no);
	list_for_each_entry(send_ctx, &recv_ctx->sender_list, node)
		pr_debug("recv: task=%d uvec=%d sender uipi id=%d\n",
			 recv_ctx->r_task->pid, recv_ctx->uvec_no,
			 send_ctx->uipi_id);

	spin_unlock_irqrestore(&recv_ctx->sender_lock, flags);

	return poll_flags;
}

#ifdef CONFIG_PROC_FS
static void uintrfd_show_fdinfo(struct seq_file *m, struct file *file)
{
	struct uintr_receiver_ctx  *recv_ctx = file->private_data;

	seq_printf(m, "uvec_no:%d\n", recv_ctx->uvec_no);
}
#endif

static const struct file_operations uintrfd_fops = {
#ifdef CONFIG_PROC_FS
	.show_fdinfo	= uintrfd_show_fdinfo,
#endif
	.release	= uintrfd_release,
	.poll		= uintrfd_poll,
	.read		= uintrfd_read,
	.llseek		= noop_llseek,
};

static int do_uintr_receive(void __user *handler, unsigned int flags)
{
	struct uintr_receiver_ctx *recv_ctx;
	int fd;
	int uvec;
	struct file *file;

	if (!uintr_arch_enabled())
		return -EINVAL;

	if (flags & UINTR_RECEIVER_DEBUG_INFO) {
		uintr_print_debug_info();
		return 0;
	}

	pr_debug("recv: task=%d flags %d handler %lx",
		 current->pid, flags, (unsigned long) handler);

	if (!handler)
		return -EINVAL;

	uvec = uintr_alloc_uvec((u64)handler);
	if (uvec < 0)
		return uvec;

	recv_ctx = kzalloc(sizeof(*recv_ctx), GFP_KERNEL);
	if (!recv_ctx)
		return -ENOMEM;
	recv_ctx->r_task = current;
	recv_ctx->uvec_no = uvec;
	INIT_LIST_HEAD(&recv_ctx->sender_list);
	spin_lock_init(&recv_ctx->sender_lock);

	fd = anon_inode_getfd("[uintrfd]", &uintrfd_fops, NULL,
			      O_RDONLY | O_CLOEXEC);

	file = fcheck(fd);

	file->private_data = (void *)recv_ctx;

	pr_debug("recv: recv_task=%llx task=%d fd private data %llx\n",
			(u64)recv_ctx->r_task,
			recv_ctx->r_task->pid,
			(u64)file->private_data);

	pr_debug("recv: assigned uvecfd=%d", fd);

	return fd;
}

/*
 * sys_uintr_receive - setup user interrupt receiver.
 */
SYSCALL_DEFINE2(uintr_receive, void __user *, handler, unsigned int, flags)

{
	return do_uintr_receive(handler, flags);
}
