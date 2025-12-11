#ifndef WORKER_H
#define WORKER_H

#include <openssl/ssl.h>

// Estrutura para conexão (pode ser HTTP ou HTTPS)
typedef struct {
    int fd;          // Socket file descriptor
    SSL *ssl;        // SSL context (NULL se for HTTP normal)
    int is_https;    // 1 se for HTTPS, 0 se for HTTP
} connection_t;

// Cada worker recebe o listen_fd (socket de escuta) do master
void worker_main(int listen_fd);

#endif