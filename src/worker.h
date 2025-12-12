#ifndef WORKER_H
#define WORKER_H

#include <openssl/ssl.h>

// Structure for connection (can be HTTP or HTTPS)
typedef struct {
    int fd;          // Socket file descriptor
    SSL *ssl;        // SSL context (NULL se for HTTP normal)
    int is_https;    // 1 se for HTTPS, 0 se for HTTP
} connection_t;

// Each worker receives the listen_fd (listening socket) and is_https_listener flag
void worker_main(int listen_fd, int is_https_listener);

#endif