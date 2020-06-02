#define _GNU_SOURCE
#include <syscall.h>
#include <errno.h>
#include <uliintrin.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/socket.h>

struct uintr_receiver receiver;
struct uintr_sender sender;

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

void __attribute__ ((interrupt)) generic_handler(struct __uli_frame *ui_frame,
                                                 unsigned long vector)
{
        printf("\n*****************************************\n");
        printf("UIPI sent using uvec %d with UITT index %d\n",
               receiver.uvec_fd, sender.target_id);
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

void receiver_unregister_syscall(struct uintr_receiver *recv_ptr)
{
        close(recv_ptr->uvec_fd);
}

void receiver_registration(struct uintr_receiver *recv_ptr)
{
	receiver.handler = generic_handler;
        receiver_register_syscall(&receiver);
        receiver_read_uvec(&receiver);
        _stui();
}

void sender_registration(struct uintr_sender *sender_ptr)
{
	sender.receiver_uvec_fd = receiver.uvec_fd;
        sender.target_fd = 0;
        sender_register_syscall(&sender);
        sender_read_target_id(&sender);
        printf("send: Sending User IPI using senduipi(%d)\n", sender.target_id);
        _senduipi(sender.target_id);
}

ssize_t sock_fd_write(int sock, void *buf, ssize_t buflen, int fd)
{
    ssize_t size;
    struct msghdr msg;
    struct iovec iov;
    union {
        struct cmsghdr cmsghdr;
        char control[CMSG_SPACE(sizeof (int))];
    } cmsgu;
    struct cmsghdr *cmsg;

    iov.iov_base = buf;
    iov.iov_len = buflen;

    msg.msg_name = NULL;
    msg.msg_namelen = 0;
    msg.msg_iov = &iov;
    msg.msg_iovlen = 1;

    if (fd != -1) {
        msg.msg_control = cmsgu.control;
        msg.msg_controllen = sizeof(cmsgu.control);

        cmsg = CMSG_FIRSTHDR(&msg);
        cmsg->cmsg_len = CMSG_LEN(sizeof (int));
        cmsg->cmsg_level = SOL_SOCKET;
        cmsg->cmsg_type = SCM_RIGHTS;

        printf ("passing fd %d\n", fd);
        *((int *) CMSG_DATA(cmsg)) = fd;
    } else {
        msg.msg_control = NULL;
        msg.msg_controllen = 0;
        printf ("not passing fd\n");
    }

    size = sendmsg(sock, &msg, 0);

    if (size < 0)
        perror ("sendmsg");
    return size;
}

ssize_t sock_fd_read(int sock, void *buf, ssize_t bufsize, int *fd)
{
    ssize_t size;

    if (fd) {
        struct msghdr msg;
        struct iovec iov;
        union {
            struct cmsghdr cmsghdr;
            char control[CMSG_SPACE(sizeof (int))];
        } cmsgu;
        struct cmsghdr *cmsg;

        iov.iov_base = buf;
        iov.iov_len = bufsize;

        msg.msg_name = NULL;
        msg.msg_namelen = 0;
        msg.msg_iov = &iov;
        msg.msg_iovlen = 1;
        msg.msg_control = cmsgu.control;
        msg.msg_controllen = sizeof(cmsgu.control);
        size = recvmsg(sock, &msg, 0);
        if (size < 0) {
            perror ("recvmsg");
            exit(1);
        }
        cmsg = CMSG_FIRSTHDR(&msg);
        if (cmsg && cmsg->cmsg_len == CMSG_LEN(sizeof(int))) {
            if (cmsg->cmsg_level != SOL_SOCKET) {
                fprintf (stderr, "invalid cmsg_level %d\n",
                     cmsg->cmsg_level);
                exit(1);
            }
            if (cmsg->cmsg_type != SCM_RIGHTS) {
                fprintf (stderr, "invalid cmsg_type %d\n",
                     cmsg->cmsg_type);
                exit(1);
            }

            *fd = *((int *)CMSG_DATA(cmsg));
            printf ("received fd %d\n", *fd);
        } else
            *fd = -1;
    } else {
        size = read (sock, buf, bufsize);
        if (size < 0) {
            perror("read");
            exit(1);
        }
    }
    return size;
}
