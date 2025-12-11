#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <openssl/ssl.h>
#include <openssl/err.h>

#include "config.h"
#include "worker.h"
#include "thread_pool.h"
#include "shared_mem.h"
#include "semaphores.h"

/**
 * @brief Ponteiro global para a memória partilhada, acessível por outros módulos.
 */
shared_data_t* shm_data = NULL;

/**
 * @brief Estrutura global com os semáforos IPC usados pelo worker.
 */
ipc_semaphores_t sems;

/**
 * @brief SSL context global para este worker
 */
static SSL_CTX *ssl_ctx = NULL;

/**
 * @brief Inicializa OpenSSL e carrega certificados
 * @return 0 em sucesso, -1 em erro
 */
static int init_ssl_context(void) {
    SSL_library_init();
    SSL_load_error_strings();
    OpenSSL_add_all_algorithms();

    ssl_ctx = SSL_CTX_new(TLS_server_method());
    if (!ssl_ctx) {
        ERR_print_errors_fp(stderr);
        return -1;
    }

    // Carregar certificado e chave privada
    if (SSL_CTX_use_certificate_file(ssl_ctx, "localhost.pem", SSL_FILETYPE_PEM) <= 0) {
        fprintf(stderr, "Worker %d: Failed to load certificate\n", getpid());
        ERR_print_errors_fp(stderr);
        return -1;
    }

    if (SSL_CTX_use_PrivateKey_file(ssl_ctx, "localhost-key.pem", SSL_FILETYPE_PEM) <= 0) {
        fprintf(stderr, "Worker %d: Failed to load private key\n", getpid());
        ERR_print_errors_fp(stderr);
        return -1;
    }

    // Verificar se a chave privada corresponde ao certificado
    if (!SSL_CTX_check_private_key(ssl_ctx)) {
        fprintf(stderr, "Worker %d: Private key does not match certificate\n", getpid());
        return -1;
    }

    printf("Worker %d: SSL initialized successfully.\n", getpid());
    return 0;
}

/**
 * @brief Função principal do processo worker. Liga-se à memória partilhada, abre os semáforos,
 *        cria o pool de threads e aceita ligações de clientes, distribuindo-as pelas threads.
 * @param listen_fd File descriptor do socket de escuta (aceita novas ligações TCP).
 */
void worker_main(int listen_fd) {
    // Attach to shared memory created by master
    shm_data = shm_create();
    if (!shm_data) {
        fprintf(stderr, "Worker %d: Failed to attach to shared memory\n", getpid());
        exit(1);
    }

    // Open existing semaphores created by master
    sems.sem_empty = sem_open("/sem_ws_empty", 0);
    sems.sem_full  = sem_open("/sem_ws_full", 0);
    sems.sem_mutex = sem_open("/sem_ws_mutex", 0);
    sems.sem_stats = sem_open("/sem_ws_stats", 0);
    sems.sem_log   = sem_open("/sem_ws_log", 0);

    if (sems.sem_empty == SEM_FAILED || sems.sem_full == SEM_FAILED || 
        sems.sem_mutex == SEM_FAILED || sems.sem_stats == SEM_FAILED ||
        sems.sem_log == SEM_FAILED) {
        perror("Worker: Failed to open semaphores");
        exit(1);
    }

    // Inicializar SSL
    if (init_ssl_context() < 0) {
        fprintf(stderr, "Worker %d: SSL initialization failed, continuing without HTTPS\n", getpid());
        ssl_ctx = NULL; // Continuar sem SSL
    }

    // criar thread pool local deste worker
    thread_pool_t pool;
    thread_pool_init(&pool, get_threads_per_worker());

    printf("Worker %d iniciado.\n", getpid());

    while (1) {
        struct sockaddr_in client_addr;
        socklen_t len = sizeof(client_addr);

        // cada worker faz accept() directamente
        int client_socket = accept(listen_fd,
                                   (struct sockaddr *)&client_addr,
                                   &len);

        if (client_socket < 0) {
            perror("worker accept");
            continue;
        }

        printf("Worker %d recebeu socket %d de %s:%d\n",
               getpid(),
               client_socket,
               inet_ntoa(client_addr.sin_addr),
               ntohs(client_addr.sin_port));

        // Criar estrutura de conexão
        connection_t *conn = malloc(sizeof(connection_t));
        if (!conn) {
            perror("malloc connection");
            close(client_socket);
            continue;
        }

        conn->fd = client_socket;
        conn->ssl = NULL;
        conn->is_https = 0;

        // Se SSL está disponível, tentar fazer handshake
        if (ssl_ctx != NULL) {
            SSL *ssl = SSL_new(ssl_ctx);
            if (ssl) {
                SSL_set_fd(ssl, client_socket);

                // Tentar handshake SSL (não bloqueante, falha se for HTTP normal)
                int ret = SSL_accept(ssl);
                if (ret > 0) {
                    // Sucesso - é uma conexão HTTPS
                    conn->ssl = ssl;
                    conn->is_https = 1;
                    printf("Worker %d: HTTPS connection established\n", getpid());
                } else {
                    // Falhou - provavelmente é HTTP normal
                    int err = SSL_get_error(ssl, ret);
                    if (err != SSL_ERROR_WANT_READ && err != SSL_ERROR_WANT_WRITE) {
                        // Não é erro temporário, assumir HTTP normal
                        SSL_free(ssl);
                        conn->ssl = NULL;
                        conn->is_https = 0;
                        printf("Worker %d: HTTP connection (SSL handshake failed, treating as plain HTTP)\n", getpid());
                    }
                }
            }
        }

        // mandar a conexão para uma thread deste worker
        thread_pool_add(&pool, conn);
    }

    // Cleanup (nunca chega aqui no loop infinito, mas boa prática)
    if (ssl_ctx) {
        SSL_CTX_free(ssl_ctx);
    }
}