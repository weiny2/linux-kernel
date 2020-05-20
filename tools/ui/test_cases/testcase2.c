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

struct uintr_receiver recv;
struct uintr_sender sender;

void __attribute__ ((interrupt)) generic_handler(struct __uli_frame *ui_frame,
                                                 unsigned long vector)
{

        printf("\n*****************************************\n");
        printf("UIPI sent using uvec %d with UITT index %d\n",
               recv.uvec_fd, sender.target_id);
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

int main() {
	cpu_set_t cpuset;

	CPU_ZERO(&cpuset);
        CPU_SET(0, &cpuset);
        sched_setaffinity(0, sizeof(cpu_set_t), &cpuset);

	recv.handler = generic_handler;
	receiver_register_syscall(&recv);
	receiver_read_uvec(&recv);

	sender.receiver_uvec_fd = recv.uvec_fd;
        sender.target_fd = 0;
        sender_register_syscall(&sender);
        sender_read_target_id(&sender);
	printf(" sending self uipi from main thread that registers receiver and sender\n");
        printf("send: Sending User IPI using senduipi(%d)\n", sender.target_id);
        _senduipi(sender.target_id);

	return 0;
}

