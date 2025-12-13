// ===================== worker.c (SSL FIXED) =====================
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <errno.h>
#include <time.h>
#include <string.h>
#include <fcntl.h>

#include <openssl/ssl.h>
#include <openssl/err.h>

#include "config.h"
#include "worker.h"
#include "thread_pool.h"
#include "shared_mem.h"
#include "semaphores.h"
#include "ssl.h"

// Global reference to the SSL_CTX created in master
extern ssl_server_ctx_t *global_ssl_ctx;

// Worker's shared memory
shared_data_t* shm_data = NULL;
ipc_semaphores_t sems;

/**
 * @brief Sets a socket as blocking
 */
static int set_blocking(int fd) {
    int flags = fcntl(fd, F_GETFL, 0);
    if (flags == -1) return -1;
    return fcntl(fd, F_SETFL, flags & ~O_NONBLOCK);
}

/**
 * @brief Tries to perform SSL handshake with timeout
 */
static int ssl_accept_with_timeout(SSL *ssl, int fd, int timeout_sec) {
    // Temporarily block the socket for SSL_accept
    set_blocking(fd);
    
    struct timeval tv;
    tv.tv_sec = timeout_sec;
    tv.tv_usec = 0;
    setsockopt(fd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));
    setsockopt(fd, SOL_SOCKET, SO_SNDTIMEO, &tv, sizeof(tv));
    
    int ret = SSL_accept(ssl);
    
    if (ret <= 0) {
        int ssl_err = SSL_get_error(ssl, ret);
        
        switch (ssl_err) {
            case SSL_ERROR_WANT_READ:
            case SSL_ERROR_WANT_WRITE:
                fprintf(stderr, "[Worker %d] SSL_accept: needs more data\n", getpid());
                return -1;
                
            case SSL_ERROR_SYSCALL:
                if (errno == ETIMEDOUT) {
                    fprintf(stderr, "[Worker %d] SSL_accept: timeout\n", getpid());
                } else {
                    fprintf(stderr, "[Worker %d] SSL_accept: syscall error (errno=%d)\n", 
                            getpid(), errno);
                    ERR_print_errors_fp(stderr);
                }
                return -1;
                
            case SSL_ERROR_SSL:
                fprintf(stderr, "[Worker %d] SSL_accept: SSL protocol error\n", getpid());
                ERR_print_errors_fp(stderr);
                return -1;
                
            case SSL_ERROR_ZERO_RETURN:
                fprintf(stderr, "[Worker %d] SSL_accept: connection closed\n", getpid());
                return -1;
                
            default:
                fprintf(stderr, "[Worker %d] SSL_accept: unknown error %d\n", 
                        getpid(), ssl_err);
                ERR_print_errors_fp(stderr);
                return -1;
        }
    }
    
    printf("[Worker %d] SSL handshake completed successfully\n", getpid());
    return 0;
}

void worker_main(int listen_fd, int is_https_listener) {
    // SHM
    shm_data = shm_attach_worker();
    if (!shm_data) {
        fprintf(stderr, "[Worker %d] Failed to attach SHM\n", getpid());
        exit(1);
    }

    // IPC SEMAPHORES
    sems.sem_accept = sem_open("/sem_ws_accept", 0);
    sems.sem_stats  = sem_open("/sem_ws_stats", 0);
    sems.sem_log    = sem_open("/sem_ws_log", 0);

    if (sems.sem_accept == SEM_FAILED ||
        sems.sem_stats == SEM_FAILED ||
        sems.sem_log   == SEM_FAILED) {
        perror("Worker sem_open");
        exit(1);
    }

        // Start thread pool
    thread_pool_t pool;
    int nthreads = get_threads_per_worker();
    thread_pool_init(&pool, nthreads);

    const char* type = is_https_listener ? "HTTPS" : "HTTP";
        printf("[Worker %d] Started with %d threads - Type: %s\n",
            getpid(), nthreads, type);

    // Check if we have SSL context available for HTTPS worker
    if (is_https_listener && !global_ssl_ctx) {
        fprintf(stderr, "[Worker %d] ERRO: Worker HTTPS sem SSL context!\n", getpid());
        exit(1);
    }

    struct sockaddr_in client_addr;
    socklen_t len = sizeof(client_addr);

    // Worker's accept loop
    while (1) {
        int client_socket = accept(listen_fd,
                                   (struct sockaddr *)&client_addr,
                                   &len);

        if (client_socket < 0) {
            if (errno == EINTR || errno == EAGAIN)
                continue;
            perror("accept");
            continue;
        }

        printf("[Worker %d] Accepted connection fd=%d from %s:%d (Type: %s)\n",
               getpid(),
               client_socket,
               inet_ntoa(client_addr.sin_addr),
               ntohs(client_addr.sin_port),
               type);

        // Create connection_t for the thread pool
        connection_t *conn = malloc(sizeof(connection_t));
        if (!conn) {
            fprintf(stderr, "[Worker %d] Error allocating connection_t\n", getpid());
            close(client_socket);
            continue;
        }
        
        conn->fd = client_socket;
        conn->is_https = is_https_listener;
        conn->ssl = NULL;

        // If HTTPS → create SSL object and perform handshake
        if (is_https_listener) {
            printf("[Worker %d] Starting SSL handshake...\n", getpid());
            
            conn->ssl = SSL_new(global_ssl_ctx->ctx);
            if (!conn->ssl) {
                fprintf(stderr, "[Worker %d] Error creating SSL object\n", getpid());
                ERR_print_errors_fp(stderr);
                close(client_socket);
                free(conn);
                continue;
            }
            
            if (SSL_set_fd(conn->ssl, client_socket) != 1) {
                fprintf(stderr, "[Worker %d] Error associating SSL to socket\n", getpid());
                ERR_print_errors_fp(stderr);
                SSL_free(conn->ssl);
                close(client_socket);
                free(conn);
                continue;
            }
            
            // Perform SSL handshake with 10 second timeout
            if (ssl_accept_with_timeout(conn->ssl, client_socket, 10) != 0) {
                fprintf(stderr, "[Worker %d] SSL handshake failed\n", getpid());
                SSL_free(conn->ssl);
                close(client_socket);
                free(conn);
                continue;
            }
            
            printf("[Worker %d] HTTPS connection established (cipher: %s)\n", 
                   getpid(), SSL_get_cipher(conn->ssl));
        }

        // Send to the thread pool
        thread_pool_add(&pool, conn);
    }
}