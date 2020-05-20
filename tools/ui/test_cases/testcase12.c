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
#include <sys/wait.h>

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

static int uvec_fd = 0;
static int flag = 0;

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

void ChildProcess(void)
{
	sender.target_fd = 0;
        sender_register_syscall(&sender);
        sender_read_target_id(&sender);
        printf("send: Sending User IPI using senduipi(%d)\n", sender.target_id);
	_senduipi(sender.target_id);
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

static void wait_for_pid(int pid)
{
    int status;
    int corpse;
    while ((corpse = wait(&status)) >= 0 && corpse != pid)
        printf("Unexpected child %d exited with status 0x%.4X\n", corpse, status);
    if (corpse == pid)
        printf("Child %d exited with status 0x%.4X\n", corpse, status);
    else
        printf("Child %d died without its death being tracked\n", pid);
}

int main() {
	int pipefd[2];
	int buf;
	pid_t  pid;
	cpu_set_t cpuset;

	CPU_ZERO(&cpuset);
        CPU_SET(0, &cpuset);
        sched_setaffinity(0, sizeof(cpu_set_t), &cpuset);

	if (pipe(pipefd) < 0)
        	exit(1);

	recv.handler = generic_handler;

	pid = fork();
	if (pid == 0) {
		read(pipefd[0], &buf, 4);
		printf("reading from pipe successfully\n");
                sender.receiver_uvec_fd = buf;
		printf(" about to register sender\n");
		//sleep(15);
                ChildProcess();
	} else {
                receiver_register_syscall(&recv);
                receiver_read_uvec(&recv);
		sleep(15);
                uvec_fd = recv.uvec_fd;
                write(pipefd[1], &uvec_fd, 4);
		printf(" wrote to pipe successfully\n");
		//sleep(15);
		//wait(NULL);
		wait_for_pid(pid);
	}

	return 0;
}

