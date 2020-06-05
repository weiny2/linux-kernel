#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdarg.h>
#include <unistd.h>
#include <fcntl.h>
#include <getopt.h>
#include <pthread.h>
#include <signal.h>
#include <sched.h>
#include <string.h>
#include <time.h>
#include <errno.h>
#include <limits.h>
#include <linux/unistd.h>
#include <sys/epoll.h>

#include <sys/prctl.h>
#include <sys/stat.h>
#include <sys/sysinfo.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <sys/utsname.h>
#include <sys/mman.h>
#include <sys/syscall.h>
#include <sys/eventfd.h>
#include <uliintrin.h>
#include <errno.h>
#include <syscall.h>

enum ep_type {
        EP_UIPI, /* UIPI */
        EP_PIPE, /* Pipe */
        EP_EFD, /* eventfd */
        EP_SIG, /*signal */
        EP_TYPE_NR,
};

/* event notification method used by receiver */
enum en_type {
        EN_UIPI,
        EN_READ,
        EN_EFD,
        EN_SIG,
        EN_NR,
};

char *ipc_type_name[] = {
        "uipi",
        "pipe",
        "eventfd",
        "signal",
};

struct event_param {
        int tx_cpu;
        int rx_cpu;
        uint64_t pi_tx;
        uint64_t pi_rx;
        uint64_t sig_rx;
        uint64_t sig_tx;
        uint64_t efd_rx;
        uint64_t efd_tx;
        uint64_t tsc;
	uint64_t event_fd;
	int pid;
	int pfd[2];
	int efd;
	char *msg;
	char *efd_msg;
        enum ep_type type;
        enum en_type notify;
};

struct uintr_receiver {
        void *handler;
        int uvec_fd;
        int uvec_no;
};

struct uintr_sender {
        int receiver_uvec_fd;
        int target_fd;
        int target_id;
};

struct uintr_receiver recv;
struct uintr_sender sender;
struct event_param par;

static inline uint64_t rdtsc()
{
        uint32_t cycles_high;
        uint32_t cycles_low;

        asm volatile("rdtscp\n\t"
                     "mov %%edx, %0\n\t"
                     "mov %%eax, %1\n\t"
                     : "=r" (cycles_high), "=r" (cycles_low)
                     : : "%rax", "%rbx", "%rcx", "%rdx");
        return( ((uint64_t)cycles_high << 32) | cycles_low) ;
}

void __attribute__ ((interrupt)) generic_handler(struct __uli_frame *ui_frame,
                                                 unsigned long vector)
{
        uint64_t tsc_stop;
        tsc_stop = rdtsc();

        printf("\n*****************************************\n");
        printf("UIPI sent using uvec %d with UITT index %d\n",
               recv.uvec_fd, sender.target_id);
        printf("UISR-UIPI:latency %lu cycles\n", tsc_stop - par.tsc);
        printf("*****************************************\n\n");
}

static void usage(const char *msg)
{
        printf("UIPI test %s", msg);
}

static uint64_t cal_latency(uint64_t t1, uint64_t t2)
{
        return (t1-t2);
}

void sender_register_syscall(struct uintr_sender *sender_ptr)
{
        int ret;
        errno = 0;

        ret = syscall(440, sender_ptr->receiver_uvec_fd, 0);
        printf("=== send: status: Registering sender with uvec_fd %d returned "
               "fd: %d with errno %d ===\n", sender_ptr->receiver_uvec_fd, ret,
               errno);
        if (ret < 0) {
                printf("send: status: Failed to register sender\n");
                exit(-1);
        }

        sender_ptr->target_fd = ret;
}

void sender_read_target_id(struct uintr_sender *sender_ptr)
{
        if(read(sender_ptr->target_fd, &sender_ptr->target_id,
                sizeof(sender_ptr->target_id)) < 0)
        {
                printf("send: reading sender target fd failed\n");
                exit(-1);
        }

        printf("send: target_id read %d\n", sender_ptr->target_id);
}

static void sig_handler(int signum)
{
        static uint64_t total;

        par.sig_rx = rdtsc();
        total = cal_latency(par.sig_rx, par.sig_tx);

	fprintf(stderr, "*************************************************\n");
        fprintf(stderr, "Average Latency of signal: %llu cycles.\n",
                (unsigned long long) total);
	fprintf(stderr, "*************************************************\n");
}

void receiver_read_uvec(struct uintr_receiver *recv_ptr)
{
        if(read(recv_ptr->uvec_fd, &recv_ptr->uvec_no, sizeof(recv_ptr->uvec_no)) < 0) {
                printf("recv: uvecfd read failed. Uintr not working\n");
                exit(-1);
        }
}

void receiver_register_syscall(struct uintr_receiver *recv_ptr)
{
        int ret;
        errno = 0;

        ret = syscall(439, recv_ptr->handler, 0);
        printf("=== recv: status: Registering receiver returned fd: %d with"
                " errno %d ===\n", ret, errno);
        if (ret < 0) {
                printf("recv: status: receiver registration failed\n");
                exit(-1);
        }

        recv_ptr->uvec_fd = ret;
}

void sender_registration(struct uintr_sender *sender_ptr)
{
	sender.receiver_uvec_fd = recv.uvec_fd;
	sender_register_syscall(&sender);
	sender_read_target_id(&sender);
	par.tsc = rdtsc();
	_senduipi(sender.target_id);
}

void receiver_registration(struct uintr_receiver *recv_ptr)
{
	recv.handler = generic_handler;
	receiver_register_syscall(&recv);
	receiver_read_uvec(&recv);
	_stui();
}

void write_to_eventfd()
{
	ssize_t s;

	par.efd = eventfd(0, 0);
	if (par.efd == -1)
		perror("ERR: create eventfd\n");

	par.efd_msg = "IPC TEST";
	par.event_fd = strtoull(par.efd_msg, NULL, 0);
	par.efd_tx = rdtsc();
	s = write(par.efd, &par.event_fd, sizeof(uint64_t));
	if (s != sizeof(uint64_t))
		perror("ERR: write eventfd\n");
}

void read_from_eventfd()
{
	uint64_t total;
	uint64_t event_fd;
	ssize_t s;

	par.efd_rx = rdtsc();
	s = read(event_fd, &par.event_fd, sizeof(uint64_t));
	if (s != sizeof(uint64_t))
		perror("ERR: read eventfd\n");
	total = cal_latency(par.efd_rx, par.efd_tx);
	fprintf(stderr, "*************************************************\n");
	fprintf(stderr, "Average Latency of eventfd: %llu cycles.\n",
		(unsigned long long) total);
	fprintf(stderr, "*************************************************\n");
	close(par.efd);
}
