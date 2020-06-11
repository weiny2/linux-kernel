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

	receiver.handler = generic_handler;
	_stui();
	_senduipi(0);

	return 0;
}
