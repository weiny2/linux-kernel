// SPDX-License-Identifier: GPL-2.0+
#define _GNU_SOURCE
#include <syscall.h>
#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <uliintrin.h>
#include <pthread.h>
#include <sched.h>
#include <poll.h>
#include <stdlib.h>
#include <stdint.h>

#define MAX_NUM_SENDERS 16
#define MAX_NUM_RECEIVERS MAX_NUM_SENDERS
#define NUM_THREADS 1

#define UINTR_RECEIVE_SYSCALL	439
#define UINTR_SEND_SYSCALL	440

#define	UINTR_RECEIVER_DEBUG_INFO	(1 << 10)
#define	UINTR_SENDER_SET_PUIR		(1 << 20)
#define	UINTR_SENDER_DEBUG_INFO		(1 << 21)

#define MIN(x, y) (((x) < (y)) ? (x) : (y))

struct ui_receiver {
	void *handler;
	int uvecfd;
	int uvec_no;
};

struct ui_sender {
	int receiver_uvecfd;
	int uipifd;
	int uipi_id;
};

unsigned long ui_count;
unsigned int num_threads = NUM_THREADS;

void __attribute__ ((interrupt)) ui_handler(struct __uli_frame *ui_frame,
						   unsigned long vector)
{
	printf("\t UI handler vector %ld\n", vector);
	ui_count++;
}

static inline void bdelay(uint64_t dl)
{
	volatile uint64_t cnt = dl << 20;

	// TODO: Fix this code sometimes gets stuck here
	while (cnt--) {
		if (cnt > 0x400000000) {
			printf("Error: delay greater than expected");
			break;
		}
		dl++;
	}
}

void loop_bdelay(unsigned int count)
{
	int i;

	for (i = 0; i < count; i++)
		bdelay(200);
}

void sender_register_syscall(struct ui_sender *sender)
{
	int ret;

	errno = 0;
	ret = syscall(UINTR_SEND_SYSCALL, sender->receiver_uvecfd, 0);
	printf("send: Syscall uintr_send REGISTER returned %d with errno %d\n",
		ret, errno);
	if (ret < 0) {
		printf("Sender register syscall error can't process further\n");
		exit(-1);
	}

	sender->uipifd = ret;
	printf("send: sender uipifd %d\n", sender->uipifd);
}

void sender_ipi_syscall(struct ui_sender *sender)
{
	int ret;

	printf("send: Sending IPI through kernel\n");
	errno = 0;
	ret = syscall(UINTR_SEND_SYSCALL, sender->receiver_uvecfd,
		      UINTR_SENDER_SET_PUIR);
	printf("send: Syscall uintr_send SET_PUIR returned %d with errno %d\n",
		ret, errno);
}

void sender_debug_syscall(void)
{
	int ret;

	errno = 0;
	ret = syscall(UINTR_SEND_SYSCALL, 0, UINTR_SENDER_DEBUG_INFO);
	printf("send: Syscall uintr_send DEBUG returned %d with errno %d\n",
		ret, errno);
}

void sender_wait_poll(struct ui_sender *sender)
{
	struct pollfd fds;
	int timeout_msecs = 500;
	int ret = 0;

	printf("send: waiting for receiver exit using poll..\n");

	fds.fd = sender->uipifd;
	fds.events = 0; //POLLOUT | POLLWRBAND | POLLIN;
	while (ret != 1) {
		ret = poll(&fds, 1, timeout_msecs);
		//printf("send: Poll syscall returned %d\n", ret);
	}
}

void sender_read_uipi_id(struct ui_sender *sender)
{
	if (read(sender->uipifd, &sender->uipi_id, sizeof(sender->uipi_id))
		< 0) {
		printf("send: uipifd read error can't process further\n");
		exit(-1);
	}

	printf("send: uipi_id read %d\n", sender->uipi_id);
}

void wait_for_user_input(void)
{
	printf("waiting for user input..");
	getchar();
}


void *sender_thread(void *arg)
{
	struct ui_sender sender;

	errno = 0;

	cpu_set_t my_set;	/* Define your cpu_set bit mask. */

	CPU_ZERO(&my_set);	/* Initialize it all to 0, no CPUs selected. */
	CPU_SET(2, &my_set);	/* set the bit that represents core 2. */
	sched_setaffinity(0, sizeof(cpu_set_t), &my_set); /* Set affinity */

	sender_debug_syscall();
	sender.receiver_uvecfd = *(int *)arg;
	printf("send: receiver uvecfd %d\n", sender.receiver_uvecfd);
	sender.uipifd = 0;

	sender_register_syscall(&sender);
	sender_read_uipi_id(&sender);
	sender_debug_syscall();

	//wait_for_user_input();

	printf("send: Sending IPI using sendipi(%d)\n", sender.uipi_id);
	_senduipi(sender.uipi_id);
	sender_debug_syscall();

	// Post IPI using kernel syscall
	sender_ipi_syscall(&sender);

	// Delay sender
	loop_bdelay(num_threads * 1);

	printf("send: Sending IPI again using sendipi(%d)\n", sender.uipi_id);
	_senduipi(sender.uipi_id);

	// Wait for receiver to exit
	sender_wait_poll(&sender);
	sender_debug_syscall();

	printf("send: Exiting sender thread\n");
	close(sender.uipifd);
	sender_debug_syscall();

	return NULL;

}

void receiver_debug_syscall(void)
{
	int ret;

	errno = 0;
	ret = syscall(UINTR_RECEIVE_SYSCALL, 0, UINTR_RECEIVER_DEBUG_INFO);
	printf("recv: Syscall uintr_receive DEBUG returned %d with errno %d\n",
		ret, errno);
}

void receiver_read_uvec(struct ui_receiver *receiver)
{
	if (read(receiver->uvecfd, &receiver->uvec_no,
		 sizeof(receiver->uvec_no)) < 0) {
		printf("recv: uvecfd read failed. Uintr not working\n");
		exit(-1);
	}

	printf("recv: uvecfd read %d\n", receiver->uvec_no);
}

void receiver_register_syscall(struct ui_receiver *receiver)
{
	int ret;

	errno = 0;
	ret = syscall(UINTR_RECEIVE_SYSCALL, receiver->handler, 0);
	printf("recv: Syscall uintr_receive REGISTER ret %d with errno %d\n",
		ret, errno);
	if (ret < 0) {
		printf("Receiver register error can't process further\n");
		exit(-1);
	}

	receiver->uvecfd = ret;
	printf("recv: uvecfd %d\n", receiver->uvecfd);

}

void receiver_wait_for_interrupt(unsigned int loop)
{
	int i;

	for (i = 0; i < loop; i++) {
		//_testui();
		if (ui_count != 0)
			break;
		printf("recv: waiting for User interrupt to get delivered\n");
		bdelay(200);
		sleep(1); // To cause a context switch for this thread
		receiver_debug_syscall();
	}
}

void sender_thread_start(pthread_t *pt, int *uvecfd)
{
	int ret = 0;

	printf("main: creating sender thread\n");
	ret = pthread_create(pt, NULL, &sender_thread, (void *)uvecfd);
	if (ret)
		printf("main: Error %d creating pthread\n", ret);

}

int main(int argc, char *argv[])
{
	struct ui_receiver recv[MAX_NUM_RECEIVERS];
	int ret = 0;
	int i = 0;
	struct pollfd fds[MAX_NUM_RECEIVERS];
	int timeout_msecs = 5000;
	pthread_t pt[MAX_NUM_SENDERS];

	cpu_set_t my_set;	/* Define your cpu_set bit mask. */

	CPU_ZERO(&my_set);	/* Initialize it all to 0, no CPUs selected. */
	CPU_SET(3, &my_set);	/* set the bit that represents core 3. */
	sched_setaffinity(0, sizeof(cpu_set_t), &my_set); /* Set affinity */

	if ((argc > 1) && (atoi(argv[1]) > 0))
		num_threads = MIN(atoi(argv[1]), MAX_NUM_RECEIVERS);

	printf("Starting uintr test with %d threads\n", num_threads);

	for (i = 0; i < num_threads; i++) {
		// Setup receiver
		recv[i].handler = ui_handler;
		receiver_register_syscall(&recv[i]);
		receiver_read_uvec(&recv[i]);
		_stui();
		receiver_debug_syscall();

		// Start Sender thread
		sender_thread_start(&pt[i], &recv[i].uvecfd);

		fds[i].fd = recv[i].uvecfd;
		fds[i].events = POLLOUT | POLLWRBAND;
	}

	receiver_debug_syscall();
	//receiver_wait_for_interrupt(-1);
	receiver_wait_for_interrupt(5);

	ret = poll(fds, num_threads, timeout_msecs);
	printf("recv: Poll syscall returned %d\n", ret);

	// Delay closing the receiver
	loop_bdelay(num_threads * 2);

	for (i = 0; i < num_threads; i++) {
		close(recv[i].uvecfd);
		receiver_debug_syscall();
	}

	printf("recv: waiting for pthread join\n");
	for (i = 0; i < num_threads; i++)
		pthread_join(pt[i], NULL);
	printf("recv: pthread join finished\n");
	//_testui();

	receiver_debug_syscall();
	printf("UI count %ld\n", ui_count);

	printf("Ending uintr syscall test with %d threads\n", num_threads);
	return 0;
}

