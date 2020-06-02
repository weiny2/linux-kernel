#define _GNU_SOURCE
#include <stdio.h>
#include <uliintrin.h>
#include <pthread.h>
#include <sched.h>
#include "testcases.h"

int receiver_flag = 0;
int sender_flag = 0;
int loop = 5;

void *sender_thread()
{
	int i;
	errno = 0;

        cpu_set_t my_set;
        CPU_ZERO(&my_set);
        CPU_SET(1, &my_set);
        sched_setaffinity(0, sizeof(cpu_set_t), &my_set);

	while(1) {
		sleep(1);
		if(receiver_flag)
			break;
	}
	sender.receiver_uvec_fd = receiver.uvec_fd;
	sender.target_fd = 0;
        sender_register_syscall(&sender);
        sender_read_target_id(&sender);
        printf("send: Sending User IPI using senduipi(%d)\n", sender.target_id);
	for(i=0; i< loop; i++) {
		_senduipi(sender.target_id);
		sleep(3);
	}
	sender_flag = 1;

        return NULL;
}

void *receiver_thread()
{
        errno = 0;

        cpu_set_t my_set;
        CPU_ZERO(&my_set);
        CPU_SET(0, &my_set);
        sched_setaffinity(0, sizeof(cpu_set_t), &my_set);

	receiver.handler = generic_handler;
        receiver_register_syscall(&receiver);
        receiver_read_uvec(&receiver);
	receiver_flag = 1;
	while(1) {
		_stui();
		sleep(1);
		//_clui();
		if(sender_flag)
			break;
	}

        return NULL;
}

int main() {
	cpu_set_t cpuset;
	pthread_t sender, recv;

	CPU_ZERO(&cpuset);
        CPU_SET(0, &cpuset);
        sched_setaffinity(0, sizeof(cpu_set_t), &cpuset);

	pthread_create(&recv, NULL, receiver_thread, NULL);
	pthread_create(&sender, NULL, sender_thread, NULL);

	pthread_join(sender, NULL);
	pthread_join(recv, NULL);

	return 0;
}
