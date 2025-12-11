// ===================== worker.c (CORRIGIDO SSL) =====================
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

// Referência global ao SSL_CTX criado no master
extern ssl_server_ctx_t *global_ssl_ctx;

// Memória partilhada do worker
shared_data_t* shm_data = NULL;
ipc_semaphores_t sems;

/**
 * @brief Define um socket como bloqueante
 */
static int set_blocking(int fd) {
    int flags = fcntl(fd, F_GETFL, 0);
    if (flags == -1) return -1;
    return fcntl(fd, F_SETFL, flags & ~O_NONBLOCK);
}

/**
 * @brief Tenta fazer SSL handshake com timeout
 */
static int ssl_accept_with_timeout(SSL *ssl, int fd, int timeout_sec) {
    // Temporariamente bloquear o socket para SSL_accept
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
                fprintf(stderr, "[Worker %d] SSL_accept: precisa de mais dados\n", getpid());
                return -1;
                
            case SSL_ERROR_SYSCALL:
                if (errno == ETIMEDOUT) {
                    fprintf(stderr, "[Worker %d] SSL_accept: timeout\n", getpid());
                } else {
                    fprintf(stderr, "[Worker %d] SSL_accept: erro syscall (errno=%d)\n", 
                            getpid(), errno);
                    ERR_print_errors_fp(stderr);
                }
                return -1;
                
            case SSL_ERROR_SSL:
                fprintf(stderr, "[Worker %d] SSL_accept: erro SSL protocol\n", getpid());
                ERR_print_errors_fp(stderr);
                return -1;
                
            case SSL_ERROR_ZERO_RETURN:
                fprintf(stderr, "[Worker %d] SSL_accept: conexão fechada\n", getpid());
                return -1;
                
            default:
                fprintf(stderr, "[Worker %d] SSL_accept: erro desconhecido %d\n", 
                        getpid(), ssl_err);
                ERR_print_errors_fp(stderr);
                return -1;
        }
    }
    
    printf("[Worker %d] SSL handshake concluído com sucesso\n", getpid());
    return 0;
}

void worker_main(int listen_fd, int is_https_listener) {
    // SHM
    shm_data = shm_attach_worker();
    if (!shm_data) {
        fprintf(stderr, "[Worker %d] Falha ao anexar SHM\n", getpid());
        exit(1);
    }

    // SEMÁFOROS IPC
    sems.sem_accept = sem_open("/sem_ws_accept", 0);
    sems.sem_stats  = sem_open("/sem_ws_stats", 0);
    sems.sem_log    = sem_open("/sem_ws_log", 0);

    if (sems.sem_accept == SEM_FAILED ||
        sems.sem_stats == SEM_FAILED ||
        sems.sem_log   == SEM_FAILED) {
        perror("Worker sem_open");
        exit(1);
    }

    // Iniciar thread pool
    thread_pool_t pool;
    int nthreads = get_threads_per_worker();
    thread_pool_init(&pool, nthreads);

    const char* type = is_https_listener ? "HTTPS" : "HTTP";
    printf("[Worker %d] Iniciado com %d threads - Tipo: %s\n",
           getpid(), nthreads, type);

    // Verificar se temos SSL context disponível para worker HTTPS
    if (is_https_listener && !global_ssl_ctx) {
        fprintf(stderr, "[Worker %d] ERRO: Worker HTTPS sem SSL context!\n", getpid());
        exit(1);
    }

    struct sockaddr_in client_addr;
    socklen_t len = sizeof(client_addr);

    // Loop accept do worker
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

        printf("[Worker %d] Aceitou conexão fd=%d de %s:%d (Tipo: %s)\n",
               getpid(),
               client_socket,
               inet_ntoa(client_addr.sin_addr),
               ntohs(client_addr.sin_port),
               type);

        // Criar connection_t para o thread pool
        connection_t *conn = malloc(sizeof(connection_t));
        if (!conn) {
            fprintf(stderr, "[Worker %d] Erro ao alocar connection_t\n", getpid());
            close(client_socket);
            continue;
        }
        
        conn->fd = client_socket;
        conn->is_https = is_https_listener;
        conn->ssl = NULL;

        // Se for HTTPS → criar SSL object e fazer handshake
        if (is_https_listener) {
            printf("[Worker %d] A iniciar handshake SSL...\n", getpid());
            
            conn->ssl = SSL_new(global_ssl_ctx->ctx);
            if (!conn->ssl) {
                fprintf(stderr, "[Worker %d] Erro ao criar SSL object\n", getpid());
                ERR_print_errors_fp(stderr);
                close(client_socket);
                free(conn);
                continue;
            }
            
            if (SSL_set_fd(conn->ssl, client_socket) != 1) {
                fprintf(stderr, "[Worker %d] Erro ao associar SSL ao socket\n", getpid());
                ERR_print_errors_fp(stderr);
                SSL_free(conn->ssl);
                close(client_socket);
                free(conn);
                continue;
            }
            
            // Fazer handshake SSL com timeout de 10 segundos
            if (ssl_accept_with_timeout(conn->ssl, client_socket, 10) != 0) {
                fprintf(stderr, "[Worker %d] SSL handshake falhou\n", getpid());
                SSL_free(conn->ssl);
                close(client_socket);
                free(conn);
                continue;
            }
            
            printf("[Worker %d] Conexão HTTPS estabelecida (cipher: %s)\n", 
                   getpid(), SSL_get_cipher(conn->ssl));
        }

        // Enviar para o thread pool
        thread_pool_add(&pool, conn);
    }
}