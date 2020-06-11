#define _GNU_SOURCE
#include <stdio.h>
#include <uliintrin.h>
#include <pthread.h>
#include <sched.h>
#include "testcases.h"

int receiver_flag = 0;
int sender_flag = 0;

void *p_thread()
{
        errno = 0;

        cpu_set_t my_set;
        CPU_ZERO(&my_set);
        CPU_SET(1, &my_set);
        sched_setaffinity(0, sizeof(cpu_set_t), &my_set);

	receiver_registration(&receiver);
	receiver_flag = 1;
	while(1){
		sleep(1);
		if(sender_flag)
			break;
	}

        return NULL;
}

int main() {
	cpu_set_t cpuset;
	pthread_t thread;

	CPU_ZERO(&cpuset);
        CPU_SET(0, &cpuset);
        sched_setaffinity(0, sizeof(cpu_set_t), &cpuset);

	pthread_create(&thread, NULL, p_thread, NULL);
	while(1) {
                sleep(1);
                if(receiver_flag)
                        break;
        }
	sender_registration(&sender);
	sleep(3);
        sender_flag = 1;
	pthread_join(thread, NULL);

	return 0;
}
