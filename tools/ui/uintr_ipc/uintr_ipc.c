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

#define BUFFERSIZE 10
static char buffer[BUFFERSIZE];
static int repetitions = -1;
enum ep_type {
	EP_PIPE,
	EP_EFD, /* eventfd */
	EP_SIG, /*signal */
	EP_EPFD, /* epoll */
	EP_TYPE_NR,
};

/* event notification method used by receiver */
enum en_type {
	EN_READ,
	EN_EPOLL,
	EN_NR,
};

char *ipc_type_name[] = {
	"pipe",
	"eventfd",
	"signal",
};

struct endpoint {
	pthread_t thread;
	pthread_attr_t attr;
	int pfd[2]; /* receiver and sender fds */
	int	pid;
	enum ep_type type;
};

struct endpoint sender_ep;
struct endpoint receiver_ep;

static pthread_mutex_t event_param_lock = PTHREAD_MUTEX_INITIALIZER;
typedef void* (*thread_f)(void* param);

struct event_param {
	int tx_ready;
	int rx_ready;
	int tx_cpu;
	int rx_cpu;
	unsigned long long stat;
	unsigned long long ctrl;
	thread_f isr;
	uint64_t ts_tx;	
	uint64_t ts_rx;
	uint64_t tsc;
	enum ep_type type;
	enum en_type notify;
};

struct event_param par;

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

        printf("\n*****************************************\n");
        printf("UIPI sent using uvec %d with UITT index %d\n",
               recv.uvec_fd, sender.target_id);
	tsc_stop = rdtsc();
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

static void print_cpu(cpu_set_t *cpuset)
{
	int j;
	for (j = 0; j < CPU_SETSIZE; j++)
		if (CPU_ISSET(j, cpuset))
			printf("    CPU %d\n", j);
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

static void *sender_thread_func(void *param)
{
	struct event_param *p = &par;
	pthread_t thread;
	cpu_set_t cpuset;
	int stat;
	thread = pthread_self();

	sender_ep.pid = getpid();
	CPU_ZERO(&cpuset);
	CPU_SET(p->tx_cpu, &cpuset);

	stat = pthread_setaffinity_np(thread, sizeof(cpu_set_t), &cpuset);
	if (stat)
		perror("Failed to set thread affinity\n");
	//sleep(10);
	printf("TX thread on:");
	print_cpu(&cpuset);
	receiver_ep.pid = getpid();

	while (repetitions) {
		int delay = 10000;
		usleep(1);
		while (!par.rx_ready) {
			if (delay--)
				continue;
			else {
				fprintf(stderr, "RX not ready\n");
				delay = 100000;
			}
		}
		pthread_mutex_lock(&event_param_lock);
		par.stat = 0xfafa;
		pthread_mutex_unlock(&event_param_lock);
		sender.receiver_uvec_fd = recv.uvec_fd;
		sender_register_syscall(&sender);
		sender_read_target_id(&sender);
		par.tsc = rdtsc();
		_senduipi(sender.target_id);

		sleep(5);
		par.ts_tx = rdtsc();
		switch(receiver_ep.type) {
		case EP_PIPE:
			write(receiver_ep.pfd[1], &p, sizeof(void*));
			break;
		case EP_EFD:
			write(receiver_ep.pfd[0], &p, sizeof(void*));
			break;
		case EP_SIG:
			kill(receiver_ep.pid, SIGUSR1);
			break;
		default:
			perror("Invalid IPC type\n");
		}
	}

/*	sender.receiver_uvec_fd = recv.uvec_fd;
	sender_register_syscall(&sender);
        sender_read_target_id(&sender);
	par.tsc = rdtsc();
	_senduipi(sender.target_id);*/

	return NULL;
}

#define MAXEVENTS 64

/* TODO: wait on fd set */
static int epoll_wait_fd(int fd)
{
	int epfd;
	struct epoll_event event;
	struct epoll_event *events;
	int status;
	
	epfd = epoll_create1 (0);
	if (epfd == -1) {
		perror("epoll_create");
		return -1;
	}

	event.data.fd = fd;
	event.events = EPOLLIN | EPOLLET;
	events = calloc (MAXEVENTS, sizeof event);
	if (epoll_ctl(epfd, EPOLL_CTL_ADD, fd, &event) == -1)
		perror ("epoll_ctl");
	status = epoll_wait (epfd, events, MAXEVENTS, -1);
	printf("epoll received %d events\n", status);

	free(events);
	return 0;
}

static void sig_handler(int signum)
{
	uint64_t diff;
	static uint64_t total;
	static int loops = 0;

	par.ts_rx = rdtsc();	
	diff = cal_latency(par.ts_rx, par.ts_tx);	
	if (signum == SIGUSR1) {
		total += diff;
		loops++;
	}

	fprintf(stderr, "Average Latency of signal: %llu cycles.\n",
		(unsigned long long) total/loops);
	exit(0);	
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

static void *receiver_thread_func(void *param)
{
	uint64_t diff;
	uint64_t total = 0;
	int loops = repetitions;
	struct event_param *idata;
	pthread_t thread;
	cpu_set_t cpuset;
	int stat;

	struct event_param *p = &par;

	thread = pthread_self();

	CPU_ZERO(&cpuset);
	CPU_SET(p->rx_cpu, &cpuset);

	stat = pthread_setaffinity_np(thread, sizeof(cpu_set_t), &cpuset);
	if (stat)
		perror("Failed to set thread affinity\n");
	printf("RX thread on:");
	print_cpu(&cpuset);
	recv.handler = generic_handler;
	receiver_register_syscall(&recv);
	receiver_read_uvec(&recv);
//	sleep(10);
	signal(SIGUSR1, sig_handler);
	while (repetitions--) {
		pthread_mutex_lock(&event_param_lock);
		par.rx_ready = 1;
		pthread_mutex_unlock(&event_param_lock);
		switch (p->notify) {
		case EN_EPOLL:
			epoll_wait_fd(receiver_ep.pfd[0]);
		case EN_READ:
			if (read(receiver_ep.pfd[0], &idata, 8) != 8)
				perror("ERR: read IPC\n");
			break;
		default:
			perror("Invalid notification type\n");
		}
		/* processing, no more events */
		pthread_mutex_lock(&event_param_lock);
		par.rx_ready = 0;
		pthread_mutex_unlock(&event_param_lock);
		par.ts_rx = rdtsc();		
		diff = cal_latency(par.ts_rx, par.ts_tx);		
		total += diff;
	}
	fprintf(stderr, "Average Latency of %s: %llu cycles.\n",
		ipc_type_name[par.type], (unsigned long long) total/loops);

	return NULL;
}


void initialize_ep(struct endpoint *ep, thread_f func)
{
	int status;

	status = pthread_attr_init(&ep->attr);
	if (status != 0)
		fprintf(stderr, "error from pthread_attr_init: %s\n", strerror(status));
	switch (par.type) {
	case EP_PIPE:
		if (pipe(ep->pfd) == -1) {
			perror("pipe");
			exit(1);
		}
		break;
	case EP_EFD:
		ep->pfd[0] = eventfd(0, 0);
		ep->pfd[1] = eventfd(0, 0);
		break;
	case EP_SIG:
		break;
	default:
		printf("type %d\n", par.type);
	}
	ep->type = par.type;

	status = pthread_create(&ep->thread, &ep->attr, func, (void *)&par);
	if (status)
		printf("failed to create thread: %s\n", strerror(status));

}

thread_f *doit(void *data)
{
	return NULL;
}

int main(int ac, char **av)
{
	int parallel = 1;
	int warmup = 0;
	int c;
	int send_cpu = -1;
	int recv_cpu = -1;

	char* msg = "[-P <parallelism>] [-W <warmup>] [-N <repetitions>][-S <sendCPU>][-R <recvCPU>]][-T <type 0 pipe, 1 efd>][-E <event 0 read, 1 epoll]\n";

	if (geteuid() != 0) {
                printf("ERR: Needs root access for PM, sched, etc.\n");
                exit(EXIT_FAILURE);
        }
	system("echo performance >  /sys/devices/system/cpu/cpu0/cpufreq/scaling_governor ");

	while (( c = getopt(ac, av, "P:W:N:S:R:T:E:")) != EOF) {
		switch(c) {
		case 'P':
			parallel = atoi(optarg);
			if (parallel <= 0) usage(msg);
			break;
		case 'W':
			warmup = atoi(optarg);
			break;
		case 'N':
			repetitions = atoi(optarg);
			break;
		case 'S':
			send_cpu = atoi(optarg);
			break;
		case 'R':
			recv_cpu = atoi(optarg);
			break;
		case 'T':
			par.type = atoi(optarg);
			if (par.type >= EP_TYPE_NR) {
				perror("ERR: Invalid IPC type, use pipe!!!\n");
				par.type = EP_PIPE;
			}
			break;
		case 'E':
			par.notify = atoi(optarg);
			if (par.notify >= EN_NR) {
				perror("ERR: Invalid Notification type, default to read!!!\n");
				par.notify = EN_READ;
			}
			break;
		default:
			break;
		}
	}
	if (ac < 2) {
		usage(msg);
		exit(EXIT_FAILURE);
	}
	if (send_cpu == recv_cpu) {
		perror("ERR: Use different CPUs for send/recv\n");
		exit(EXIT_FAILURE);
	}
	printf("Test repeats %d after warmup %d\n", repetitions, warmup);
	par.isr = (thread_f)&doit;
	par.tx_cpu = send_cpu;
	par.rx_cpu = recv_cpu;
	initialize_ep(&receiver_ep, sender_thread_func);
	initialize_ep(&sender_ep, receiver_thread_func);

	printf("Press Enter to stop: \n");
	while(fgets(buffer, BUFFERSIZE , stdin) != NULL)
		break;

	return (0);
}

