#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <netinet/in.h>
#include <fcntl.h>
#include <errno.h>

#include <openssl/ssl.h>
#include <openssl/err.h>

#include "config.h"
#include "worker.h"
#include "thread_pool.h"
#include "shared_mem.h"
#include "semaphores.h"
#include "ssl.h"
#include "global.h"
#include "fd_passing.h"

static int set_blocking(int fd) {
    int flags = fcntl(fd, F_GETFL, 0);
    if (flags == -1) return -1;
    return fcntl(fd, F_SETFL, flags & ~O_NONBLOCK);
}

static int ssl_accept_with_timeout(SSL *ssl, int fd, int timeout_sec) {
    set_blocking(fd);
    
    struct timeval tv;
    tv.tv_sec = timeout_sec;
    tv.tv_usec = 0;
    setsockopt(fd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));
    setsockopt(fd, SOL_SOCKET, SO_SNDTIMEO, &tv, sizeof(tv));
    
    int ret = SSL_accept(ssl);
    
    if (ret <= 0) {
        int ssl_err = SSL_get_error(ssl, ret);
        fprintf(stderr, "[Worker %d] SSL_accept failed: error=%d\n", getpid(), ssl_err);
        ERR_print_errors_fp(stderr);
        return -1;
    }
    
    return 0;
}

void worker_main(int unix_sock) {
    // Attach shared memory
    shm_data = shm_attach_worker();
    if (!shm_data) {
        fprintf(stderr, "[Worker %d] Failed to attach SHM\n", getpid());
        exit(1);
    }

    // Open semaphores
    sems.sem_queue_mutex = sem_open("/sem_ws_queue_mutex", 0);
    sems.sem_queue_empty = sem_open("/sem_ws_queue_empty", 0);
    sems.sem_queue_full  = sem_open("/sem_ws_queue_full", 0);
    sems.sem_stats       = sem_open("/sem_ws_stats", 0);
    sems.sem_log         = sem_open("/sem_ws_log", 0);

    if (sems.sem_queue_mutex == SEM_FAILED ||
        sems.sem_queue_empty == SEM_FAILED ||
        sems.sem_queue_full == SEM_FAILED ||
        sems.sem_stats == SEM_FAILED ||
        sems.sem_log == SEM_FAILED) {
        perror("Worker sem_open");
        exit(1);
    }

    // Start thread pool
    thread_pool_t pool;
    int nthreads = get_threads_per_worker();
    thread_pool_init(&pool, nthreads);

    printf("[Worker %d] Started with %d threads (CONSUMER mode)\n", 
           getpid(), nthreads);

    // Worker CONSUMER loop
    while (1) {
        // Receive file descriptor from master
        fd_metadata_t meta;
        int client_fd = recv_fd(unix_sock, &meta);
        
        if (client_fd < 0) {
            fprintf(stderr, "[Worker %d] Failed to receive FD, exiting\n", getpid());
            break;
        }
        
        printf("[Worker %d] Received connection fd=%d (type=%s) from %s:%d\n",
               getpid(), client_fd, meta.is_https ? "HTTPS" : "HTTP",
               meta.client_ip, meta.client_port);

        // Create connection_t for thread pool
        connection_t *conn = malloc(sizeof(connection_t));
        if (!conn) {
            fprintf(stderr, "[Worker %d] Memory allocation error\n", getpid());
            close(client_fd);
            continue;
        }
        
        conn->fd = client_fd;
        conn->is_https = meta.is_https;
        conn->ssl = NULL;

        // If HTTPS → create SSL object and perform handshake
        if (meta.is_https) {
            if (!global_ssl_ctx) {
                fprintf(stderr, "[Worker %d] ERROR: No SSL context available\n", getpid());
                close(client_fd);
                free(conn);
                continue;
            }
            
            conn->ssl = SSL_new(global_ssl_ctx->ctx);
            if (!conn->ssl) {
                fprintf(stderr, "[Worker %d] Error creating SSL object\n", getpid());
                ERR_print_errors_fp(stderr);
                close(client_fd);
                free(conn);
                continue;
            }
            
            if (SSL_set_fd(conn->ssl, client_fd) != 1) {
                fprintf(stderr, "[Worker %d] Error associating SSL to socket\n", getpid());
                ERR_print_errors_fp(stderr);
                SSL_free(conn->ssl);
                close(client_fd);
                free(conn);
                continue;
            }
            
            if (ssl_accept_with_timeout(conn->ssl, client_fd, 10) != 0) {
                fprintf(stderr, "[Worker %d] SSL handshake failed\n", getpid());
                SSL_free(conn->ssl);
                close(client_fd);
                free(conn);
                continue;
            }
            
            printf("[Worker %d] HTTPS connection established (cipher: %s)\n", 
                   getpid(), SSL_get_cipher(conn->ssl));
        }

        // Send to thread pool
        thread_pool_add(&pool, conn);
    }
    
    close(unix_sock);
}