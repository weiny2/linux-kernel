#define _GNU_SOURCE
#include <stdio.h>
#include <uliintrin.h>
#include <pthread.h>
#include <sched.h>
#include "testcases.h"

int main() {
	cpu_set_t cpuset;

	CPU_ZERO(&cpuset);
        CPU_SET(0, &cpuset);
        sched_setaffinity(0, sizeof(cpu_set_t), &cpuset);

	receiver_registration(&receiver);
	sender_registration(&sender);
	sleep(3);

	return 0;
}
