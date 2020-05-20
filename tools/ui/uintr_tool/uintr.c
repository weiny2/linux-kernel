// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * uintr.c userspace UIPI test suite (User Inter Process Interrupt)
 *
 * Copyright (C) 2019 Intel Corporation. All rights reserved.
 *
 * Author: Gayatri Kammela <gayatri.kammela.@intel.com>
 */

#define _GNU_SOURCE
#include <syscall.h>
#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <uliintrin.h>
#include <pthread.h>
#include <sched.h>
#include <stdlib.h>
#include <sys/sysinfo.h>

#define MAX_CONN (2 << 14)

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

struct reg_recv {
	unsigned int uvec_fd;
	struct uintr_receiver recv;
	int senduipi_flag;
	int recv_status_flag;
	int recv_th_exit_flag;
	int recv_th_status_flag;
	int send_th_exit_flag;
	int index;
	unsigned long tsc;
};

struct uipi_info {
	int loops;
	int delays;
	int senders;
	int recvs;
	int cpus;
	int senders_per_thread;
	int recvs_per_thread;
	int cpu_topology;
	int cross_soc_lat;
};

struct reg_recv num_recv[MAX_CONN];
struct uipi_info uipi_num;
int j=0, k=0, l=0, m=0, n=0;

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

static inline void bdelay(uint64_t dl)
{
	volatile uint64_t cnt = dl << 10;

	while(cnt--)
		dl++;
}

int cross_cpu()
{
	int nprocs;
	int randomnumber;
	nprocs = get_nprocs_conf();
	if(uipi_num.cpu_topology)
		srand(time(NULL));
	randomnumber = rand() % nprocs;

	return randomnumber;
}

void __attribute__ ((interrupt)) generic_handler(struct __uli_frame *ui_frame,
                                                 unsigned long vector)
{
	unsigned long tsc_stop;

	printf("\n*****************************************\n");
	tsc_stop = rdtsc();
	printf("UIPI sent using uvec %d with UITT index %d\n",
	       num_recv[k].recv.uvec_fd, num_recv[j].index);
	printf("UISR-UIPI:latency %lu cycles\n", tsc_stop - num_recv[j].tsc);
	printf("*****************************************\n\n");
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

void *sender_thread()
{
        struct uintr_sender sender;
        errno = 0;

	cpu_set_t my_set;
	CPU_ZERO(&my_set);
	CPU_SET(cross_cpu(), &my_set);
	sched_setaffinity(0, sizeof(cpu_set_t), &my_set);

	for(int i=0; i<uipi_num.senders_per_thread; i++, j++) {
		num_recv[l].senduipi_flag = 0;
		while(1) {
			sleep(1);
			if(num_recv[k].recv_status_flag)
				break;
		}

		sender.receiver_uvec_fd = num_recv[m].recv.uvec_fd;
		sender.target_fd = 0;
		sender_register_syscall(&sender);
		sender_read_target_id(&sender);
		num_recv[j].index = sender.target_id;

		printf("send: Sending User IPI using senduipi(%d)\n",
		       sender.target_id);
		num_recv[j].tsc = rdtsc();
		for(int i=0; i< uipi_num.loops; i++) {
			_senduipi(sender.target_id);
			sleep(3);
			bdelay(uipi_num.delays);
		}
	}
	num_recv[l].senduipi_flag = 1;

	return NULL;
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

static void *receiver_thread()
{
	cpu_set_t my_set;
	CPU_ZERO(&my_set);
	CPU_SET(cross_cpu(), &my_set);
	sched_setaffinity(0, sizeof(cpu_set_t), &my_set);

	printf("-----------------------------------------------------------\n");

	for(int i=0; i<uipi_num.recvs_per_thread; i++, k++) {
		num_recv[m].recv.handler = generic_handler;
		receiver_register_syscall(&num_recv[m].recv);
		num_recv[k].recv_status_flag = 1;
		while(1) {
			sleep(1);
			if(num_recv[n].send_th_exit_flag)
				break;
		}
		receiver_read_uvec(&num_recv[m].recv);
		num_recv[n].recv_th_status_flag = 1;
		n++;
	}
	num_recv[m].recv_th_exit_flag = 1;
	printf("-----------------------------------------------------------\n");

	return NULL;
}

int main(int argc, char** argv) {
	int c, x_soc_latency=0, snum = 0, send_th = 0, recv_th = 0, delays = 0;
	int loops = 0, recvs = 0, i=0, p=0, cpu_t=0, idx=0;
	pthread_t sender, receiver;

	while((c = getopt (argc, argv, "s:S:t:T:D:d:l:L:r:R:C:p:vVhH?:")) != EOF)
	switch (c) {
	case 's':
	case 'S':
		snum = atoi(optarg);
		if (snum <= 0 || snum > MAX_CONN) {
			printf("Invalid sender!, should be "
			       "(1<s<2^14)\n");
			goto usage;
		}
		continue;
	case 't':
		send_th = atoi(optarg);
                if (send_th <= 0 || send_th > MAX_CONN) {
                        printf("Invalid number of sender threads!, "
                               "should be (1<t<2^14)\n");
                        goto usage;
                }
                continue;
	case 'T':
		recv_th = atoi(optarg);
		if (recv_th <= 0 || recv_th > MAX_CONN) {
			printf("Invalid number of threads!, "
			      "should be (1<t<2^14)\n");
			goto usage;
		}
		continue;
	case 'd':
	case 'D':
		delays = atoi(optarg);
		if (delays <= 0){
			printf("Invalid delay time!, should be "
			      "greater than 0\n");
			goto usage;
		}
		continue;
	case 'l':
	case 'L':
		loops = atoi(optarg);
		if (loops <= 0) {
			printf("Invalid loops!, should be "
			      "greater than 0\n");
			goto usage;
		}
		continue;
	case 'r':
        case 'R':
                recvs = atoi(optarg);
                if(recvs <=0 || recvs > MAX_CONN) {
                        printf("Invalid number of recvs!, "
                               "should be (1<recvs<2^14)\n");
                        goto usage;
                }
                continue;
	case 'C':
		x_soc_latency = atoi(optarg);
		continue;
	case 'p':
		cpu_t = atoi(optarg);
		continue;
	case 'v':
	case 'V':
        case '?':
        case 'h':
        default:
usage:
		fprintf (stderr,
                        "usage: %s [options]\n"
                        "Options:\n"
			"\t-S -s\tnumber of senders\tMax: 2^14\n"
                        "\t-t\tnumber of senders per thread\tDefaults to 1\tMax: 2^14\n"
                        "\t-T\tnumber of receivers per thread\tDefaults to 1\tMax: 2^14\n"
                        "\t-R -r\tnumber of receivers\tMax: 2^14\n"
                        "\t-L -l\tsend user interrupts in a loop (for stress test)\n"
                        "\t-D -d\tdelay\n"
			"\t-v -V -h -H\t prints this menu\n"
                        "Optional arguments:\n"
                        "\t-C cross socket latency; 1 to set\n"
                        "\t-p CPU topology; 1 to set\n",
                        argv[0]);
                return 1;

	}

	if (argc <= 1)
		goto usage;

	if (send_th * recv_th * snum * recvs > MAX_CONN) {
		printf(" The selection of senders, receivers, sender threads "
		       "and receiver threads exceeded the maximum connection "
		       "possible for this testcase to work. Please make "
		       "selection within the given range\n");
		exit(0);
	}

	int status = 1;
	printf("\nThis system has %d socket(s) with %d processors configured "
	       "and %d processors available.\n", status, get_nprocs_conf(),
	       get_nprocs());
	cpu_set_t my_set;
	CPU_ZERO(&my_set);
	CPU_SET(cross_cpu(), &my_set);
	sched_setaffinity(0, sizeof(cpu_set_t), &my_set);

	uipi_num.loops = loops;
	uipi_num.delays = delays;
	uipi_num.senders_per_thread = send_th;
	uipi_num.recvs_per_thread = recv_th;
	uipi_num.senders = snum;
	uipi_num.recvs = recvs;
	uipi_num.cpu_topology = cpu_t;
	uipi_num.cross_soc_lat = x_soc_latency;

	printf("You have entered\n");
	printf("__________________________________________________________\n");
	printf("loops : %d\n", uipi_num.loops);
	printf("delays : %d\n", uipi_num.delays);
	printf("senders_per_thread : %d\n", uipi_num.senders_per_thread);
	printf("recvs_per_thread : %d\n", uipi_num.recvs_per_thread);
	printf("senders : %d\n", uipi_num.senders);
	printf("recvs : %d\n", uipi_num.recvs);
	printf("cpu_topology is set to : %d\n", uipi_num.cpu_topology);
	printf("cross_socket_latency is set to : %d\n", uipi_num.cross_soc_lat);

	if(x_soc_latency == 1)
		printf("Cross socket latency is not possible on this machine "
		       "equipped with only %d sockets\n", status);

	for(i=0; i<recvs; i++,m++) {
		pthread_create(&receiver, NULL, receiver_thread, NULL);
		for(idx=0; idx < recv_th; idx++) {
			for(p=0; p<snum; p++,l++) {
				pthread_create(&sender, NULL, sender_thread,
					       NULL);
				/*
				 * sleep before you create another sender
				 * thread
				 */
				while(1) {
					sleep(1);
					if(num_recv[l].senduipi_flag)
						break;
				}
				l++;
			}
			num_recv[n].send_th_exit_flag = 1;
			while(1) {
				sleep(1);
				if(num_recv[n-1].recv_th_status_flag)
					break;
			}
		}
		/* Sleep before you create an another receiver thread */
		while(1) {
			sleep(1);
			if(num_recv[m].recv_th_exit_flag)
				break;
		}
	}

	for(int i=0; i<recvs; i++)
                pthread_join(receiver, NULL);

        for(int i=0; i<snum; i++) {
                pthread_join(sender, NULL);
        }

	return 0;
}
