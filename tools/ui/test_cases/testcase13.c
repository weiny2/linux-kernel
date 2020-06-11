#define _GNU_SOURCE
#include <stdio.h>
#include <uliintrin.h>
#include <pthread.h>
#include <sched.h>
#include <sys/socket.h>
#include "testcases.h"

void child_1(int sock)
{
    int fd;
    char buf[16];
    ssize_t size;

    sleep(1);
    for (;;) {
        size = sock_fd_read(sock, buf, sizeof(buf), &fd);
        if (size <= 0)
            break;
        printf ("read %d\n", size);
	sender.receiver_uvec_fd = fd;
        sender.target_fd = 0;
        sender_register_syscall(&sender);
        sender_read_target_id(&sender);
        printf("send: Sending User IPI using senduipi(%d)\n", sender.target_id);
        _senduipi(sender.target_id);
        if (fd != -1) {
            write(fd, "hello, world\n", 13);
            close(fd);
        }
    }
}

void child_2(int sock)
{
    ssize_t size;
    int i;
    int fd;

    receiver_registration(&receiver);
    fd = receiver.uvec_fd;
    size = sock_fd_write(sock, "1", 1, fd);
    printf ("wrote %d\n", size);
    sleep(5);
}

int main()
{
    int sv[2];
    int n1, n2;

    if (socketpair(AF_LOCAL, SOCK_STREAM, 0, sv) < 0) {
        perror("socketpair");
        exit(1);
    }

    n1 = fork();
    n2 = fork();

    if (n1 == 0 && n2 > 0)
    {
        close(sv[0]);
        child_1(sv[1]);
     } else if (n1 > 0 && n2 == 0) {
	close(sv[1]);
        child_2(sv[0]);
     } else if (n1 > 0 && n2 > 0) { 
        printf("parent\n");
	sleep(5);
     } else
     	printf("third child\n"); 	     

    return 0;
}
