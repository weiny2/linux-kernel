#define _GNU_SOURCE
#include <stdio.h>
#include <uliintrin.h>
#include <pthread.h>
#include <sched.h>
#include "testcases.h"

struct uintr_sender send_1[2];

int main() {
	int i;
	cpu_set_t cpuset;

	CPU_ZERO(&cpuset);
        CPU_SET(0, &cpuset);
        sched_setaffinity(0, sizeof(cpu_set_t), &cpuset);

	receiver_registration(&receiver);

	for(i=0; i<2; i++) {
		close(send_1[i].receiver_uvec_fd);
		send_1[i].receiver_uvec_fd = receiver.uvec_fd;
        	send_1[i].target_fd = 0;
        	sender_register_syscall(&send_1[i]);
        	sender_read_target_id(&send_1[i]);
		printf("sending self uipi from main thread that registers receiver and sender\n");
        	printf("send: Sending User IPI using senduipi(%d)\n", send_1[i].target_id);
        	_senduipi(send_1[i].target_id);
		sleep(3);
	}

	return 0;
}
