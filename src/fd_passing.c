#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <errno.h>

#include "fd_passing.h"

int create_fd_passing_pair(int sv[2]) {
    return socketpair(AF_UNIX, SOCK_STREAM, 0, sv);
}

int send_fd(int unix_sock, int fd_to_send, fd_metadata_t *meta) {
    struct msghdr msg = {0};
    struct iovec iov[1];
    
    // Buffer for control message (contains the FD)
    union {
        char buf[CMSG_SPACE(sizeof(int))];
        struct cmsghdr align;
    } u;
    
    // Data to send along with FD
    iov[0].iov_base = meta;
    iov[0].iov_len = sizeof(fd_metadata_t);
    
    msg.msg_iov = iov;
    msg.msg_iovlen = 1;
    msg.msg_control = u.buf;
    msg.msg_controllen = sizeof(u.buf);
    
    struct cmsghdr *cmsg = CMSG_FIRSTHDR(&msg);
    cmsg->cmsg_level = SOL_SOCKET;
    cmsg->cmsg_type = SCM_RIGHTS;
    cmsg->cmsg_len = CMSG_LEN(sizeof(int));
    
    memcpy(CMSG_DATA(cmsg), &fd_to_send, sizeof(int));
    
    if (sendmsg(unix_sock, &msg, 0) < 0) {
        perror("sendmsg");
        return -1;
    }
    
    return 0;
}

int recv_fd(int unix_sock, fd_metadata_t *meta) {
    struct msghdr msg = {0};
    struct iovec iov[1];
    
    union {
        char buf[CMSG_SPACE(sizeof(int))];
        struct cmsghdr align;
    } u;
    
    iov[0].iov_base = meta;
    iov[0].iov_len = sizeof(fd_metadata_t);
    
    msg.msg_iov = iov;
    msg.msg_iovlen = 1;
    msg.msg_control = u.buf;
    msg.msg_controllen = sizeof(u.buf);
    
    ssize_t n = recvmsg(unix_sock, &msg, 0);
    
    if (n < 0) {
        perror("recvmsg");
        return -1;
    }
    
    if (n == 0) {
        // Connection closed
        return -1;
    }
    
    struct cmsghdr *cmsg = CMSG_FIRSTHDR(&msg);
    
    if (!cmsg || cmsg->cmsg_type != SCM_RIGHTS) {
        fprintf(stderr, "recv_fd: invalid control message\n");
        return -1;
    }
    
    int received_fd;
    memcpy(&received_fd, CMSG_DATA(cmsg), sizeof(int));
    
    return received_fd;
}