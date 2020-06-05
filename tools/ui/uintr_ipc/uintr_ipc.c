#define _GNU_SOURCE
#include <stdio.h>
#include <uliintrin.h>
#include <pthread.h>
#include <sched.h>
#include "uintr_ipc.h"

int receiver_flag = 0;
int sender_flag = 0;
int pipe_flag = 0;
int efd_flag = 0;
int sig_flag = 0;

static void *sender_thread()
{
	cpu_set_t my_set;
	CPU_ZERO(&my_set);
	CPU_SET(1, &my_set);
	sched_setaffinity(0, sizeof(cpu_set_t), &my_set);

	if (par.type == EP_UIPI) {
		while(1) {
			sleep(1);
			if(receiver_flag)
				break;
		}
	}

	switch(par.type) {
	case EP_UIPI:
		sender_registration(&sender);
		break;
	case EP_PIPE:
		par.msg = "IPC TEST";
		par.pi_tx = rdtsc();
		write(par.pfd[1], par.msg, sizeof(par.msg));
		pipe_flag = 1;
		break;
	case EP_EFD:
		write_to_eventfd();
		efd_flag = 1;
		break;
	case EP_SIG:
		par.sig_tx = rdtsc();
		kill(par.pid, SIGUSR1);
		sig_flag = 1;
		break;
	default:
		perror("Invalid IPC type\n");
	}

	sender_flag = 1;

	return NULL;
}

static void *receiver_thread()
{
	char *data;
	uint64_t total;
	cpu_set_t my_set;
	CPU_ZERO(&my_set);
	CPU_SET(0, &my_set);
	sched_setaffinity(0, sizeof(cpu_set_t), &my_set);

	if(par.notify == EN_READ || par.notify == EN_EFD) {
		while(1) {
			sleep(1);
			if(pipe_flag || efd_flag)
				break;
		}
	}

	switch (par.notify) {
	case EN_UIPI:
		receiver_registration(&recv);
		receiver_flag = 1;
		break;
	case EN_READ:
		par.pi_rx = rdtsc();
		if (read(par.pfd[0], &data, sizeof(data)) != 8)
			perror("ERR: read IPC\n");
		total = cal_latency(par.pi_rx, par.pi_tx);
		fprintf(stderr, "*************************************************\n");
		fprintf(stderr, "Average Latency of pipe: %llu cycles.\n",
			(unsigned long long) total);
		fprintf(stderr, "*************************************************\n");
		break;
	case EN_EFD:
		read_from_eventfd();
		break;
	case EN_SIG:
		par.sig_rx = rdtsc();
		signal(SIGUSR1, sig_handler);
		break;
	default:
		perror("Invalid notification type\n");
	}
	while(1) {
		sleep(1);
		if(sender_flag)
			break;
	}

	return NULL;
}

int main(int ac, char **av)
{
	int c;
	int send_cpu = -1;
	int recv_cpu = -1;
	cpu_set_t cpuset;
        pthread_t sender, recv;

	char* msg = "[-S <sendCPU>][-R <recvCPU>]][-T <type 0 uipi, 1 pipe, 2 eventfd, 3 signal>]\n";

	if (geteuid() != 0) {
                printf("ERR: Needs root access for PM, sched, etc.\n");
                exit(EXIT_FAILURE);
        }
	system("echo performance >  /sys/devices/system/cpu/cpu0/cpufreq/scaling_governor ");

	while (( c = getopt(ac, av, "S:R:T:")) != EOF) {
		switch(c) {
		case 'S':
			send_cpu = atoi(optarg);
			break;
		case 'R':
			recv_cpu = atoi(optarg);
			break;
		case 'T':
			par.type = atoi(optarg);
			if (par.type >= EP_TYPE_NR) {
				perror("ERR: Invalid IPC type, use uipi!!!\n");
				par.type = EP_UIPI;
				par.notify = EN_UIPI;
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
	par.tx_cpu = send_cpu;
	par.rx_cpu = recv_cpu;
	switch(par.type) {
		case EP_UIPI:
			par.notify = EN_UIPI;
			break;
		case EP_PIPE:
			par.notify = EN_READ;
			break;
		case EP_EFD:
			par.notify = EN_EFD;
			break;
		case EP_SIG:
			par.notify = EN_SIG;
			break;
		default:
			break;
	}

	CPU_ZERO(&cpuset);
        CPU_SET(0, &cpuset);
        sched_setaffinity(0, sizeof(cpu_set_t), &cpuset);

	pthread_create(&recv, NULL, receiver_thread, NULL);
        pthread_create(&sender, NULL, sender_thread, NULL);

	pthread_join(sender, NULL);
        pthread_join(recv, NULL);

	return (0);
}
