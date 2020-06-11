#define _GNU_SOURCE
#include <stdio.h>
#include <uliintrin.h>
#include <pthread.h>
#include <sched.h>
#include "testcases.h"

int receiver_flag = 0;
int sender_flag = 0;

void *sender_thread()
{
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

	sender_registration(&sender);
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

	receiver_registration(&receiver);
	_clui();
        printf(" Clearing the ui bit before sending the _senduipi\n");
	receiver_flag = 1;

	while(1) {
		sleep(1);
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
