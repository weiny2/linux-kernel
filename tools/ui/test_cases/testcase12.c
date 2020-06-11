#define _GNU_SOURCE
#include <stdio.h>
#include <uliintrin.h>
#include <pthread.h>
#include <sched.h>
#include <sys/socket.h>
#include "testcases.h"

void child(int sock)
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

void parent(int sock)
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
    int pid;

    if (socketpair(AF_LOCAL, SOCK_STREAM, 0, sv) < 0) {
        perror("socketpair");
        exit(1);
    }
    switch ((pid = fork())) {
    case 0:
        close(sv[0]);
        child(sv[1]);
        break;
    case -1:
        perror("fork");
        exit(1);
    default:
        close(sv[1]);
        parent(sv[0]);
        break;
    }

    return 0;
}
