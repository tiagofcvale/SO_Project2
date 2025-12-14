#ifndef WORKER_H
#define WORKER_H

#include <openssl/ssl.h>

// Structure for connection (can be HTTP or HTTPS)
typedef struct {
    int fd;          // Socket file descriptor
    SSL *ssl;        // SSL context (NULL if plain HTTP)
    int is_https;    // 1 if HTTPS, 0 if HTTP
} connection_t;

// Worker main function - receives FDs through Unix socket
void worker_main(int unix_sock);

#endif