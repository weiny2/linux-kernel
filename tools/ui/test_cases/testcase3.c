#define _GNU_SOURCE
#include <stdio.h>
#include <uliintrin.h>
#include <pthread.h>
#include <sched.h>
#include "testcases.h"

void *f_thread()
{
	errno = 0;

        cpu_set_t my_set;
        CPU_ZERO(&my_set);
        CPU_SET(1, &my_set);
        sched_setaffinity(0, sizeof(cpu_set_t), &my_set);

	receiver_registration(&receiver);
	sender_registration(&sender);
	sleep(3);

        return NULL;
}

int main() {
	cpu_set_t cpuset;
	pthread_t p_thread;

	CPU_ZERO(&cpuset);
        CPU_SET(0, &cpuset);
        sched_setaffinity(0, sizeof(cpu_set_t), &cpuset);

	pthread_create(&p_thread, NULL, f_thread, NULL);

//	sleep(3);
	pthread_join(p_thread, NULL);

	return 0;
}
