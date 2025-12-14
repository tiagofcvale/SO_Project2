#ifndef FD_PASSING_H
#define FD_PASSING_H

// Structure to pass connection info along with FD
typedef struct {
    int is_https;
    char client_ip[64];
    int client_port;
} fd_metadata_t;

// Create Unix domain socket pair for passing FDs
int create_fd_passing_pair(int sv[2]);

// Send a file descriptor through Unix socket
int send_fd(int unix_sock, int fd_to_send, fd_metadata_t *meta);

// Receive a file descriptor through Unix socket
int recv_fd(int unix_sock, fd_metadata_t *meta);

#endif