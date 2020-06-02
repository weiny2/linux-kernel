#define _GNU_SOURCE
#include <stdio.h>
#include <uliintrin.h>
#include <pthread.h>
#include <sched.h>
#include "testcases.h"

void *sender_thread()
{
	errno = 0;

        cpu_set_t my_set;
        CPU_ZERO(&my_set);
        CPU_SET(1, &my_set);
        sched_setaffinity(0, sizeof(cpu_set_t), &my_set);

	sender_registration(&sender);

        return NULL;
}

int main() {
	cpu_set_t cpuset;
	pthread_t sender;

	CPU_ZERO(&cpuset);
        CPU_SET(0, &cpuset);
        sched_setaffinity(0, sizeof(cpu_set_t), &cpuset);

	receiver.handler = generic_handler;
	printf(" assign some random uvec_fd to test this case\n");
	receiver.uvec_fd = 3;
	_stui();

	pthread_create(&sender, NULL, sender_thread, NULL);

	pthread_join(sender, NULL);

	return 0;
}
