// ========================= master.c (FINAL) ==============================

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <errno.h>

#include "config.h"
#include "shared_mem.h"
#include "semaphores.h"
#include "worker.h"
#include "logger.h"
#include "cache.h"
#include "ssl.h"

// GLOBAL SSL CONTEXT (VISÍVEL NOS WORKERS)
ssl_server_ctx_t *global_ssl_ctx = NULL;

// ================================================================
// função de criação de socket com bind() + listen()
// ================================================================
static int create_listener(int port)
{
    int fd = socket(AF_INET, SOCK_STREAM, 0);
    if (fd < 0) {
        perror("socket");
        exit(1);
    }

    int yes = 1;
    setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes));

    struct sockaddr_in addr = {0};
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(port);

    if (bind(fd, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        perror("bind");
        exit(1);
    }

    if (listen(fd, 256) < 0) {
        perror("listen");
        exit(1);
    }

    return fd;
}


// ================================================================
//             INICIALIZAÇÃO DO MASTER PROCESS
// ================================================================
static void master_init(void)
{
    logger_init();
    cache_init(get_cache_size_mb());
    shm_create_master();
}


// ================================================================
//                 LANÇAR WORKERS (HTTP + HTTPS)
// ================================================================
static void launch_workers(int listen_http, int listen_https)
{
    int n = get_num_workers();

    printf("[MASTER] A lançar %d workers...\n", n);

    for (int i = 0; i < n; i++) {

        pid_t pid = fork();

        if (pid < 0) {
            perror("fork");
            exit(1);
        }

        if (pid == 0) {
            // FILHO = WORKER

            // fecha listeners que não usa
            // este worker servirá APENAS HTTP ou APENAS HTTPS
            if (i % 2 == 0) {
                close(listen_https);
                worker_main(listen_http, 0);   // worker HTTP
            } else {
                close(listen_http);
                worker_main(listen_https, 1);  // worker HTTPS
            }

            exit(0);
        }
    }
}


// ================================================================
// SIGNAL HANDLERS (CTRL + C)
// ================================================================
static void sigint_handler(int sig)
{
    (void)sig;
    printf("\n[MASTER] Encerrando servidor...\n");
}


// ================================================================
// MAIN
// ================================================================
int master_start(void)
{
    // Carrega config
    if (load_config("server.conf") < 0) {
        fprintf(stderr, "Erro ao ler server.conf\n");
        return 1;
    }

    int http_port  = get_server_port();
    int https_port = get_https_port();

    printf("[MASTER] ========================================\n");
    printf("[MASTER] Servidor Web com SSL/HTTPS\n");
    printf("[MASTER] ========================================\n");


    // 1) Inicializar master
    master_init();

    // 1.1) Inicializar semáforos IPC
    static ipc_semaphores_t sems;
    if (sem_init_ipc(&sems, get_num_workers()) != 0) {
        fprintf(stderr, "[MASTER] Erro ao inicializar semáforos IPC\n");
        return 1;
    }

    // 2) Criar sockets
    int listen_http  = create_listener(http_port);
    int listen_https = create_listener(https_port);

    printf("[MASTER] HTTP  aberto em 0.0.0.0:%d\n", http_port);
    printf("[MASTER] HTTPS aberto em 0.0.0.0:%d\n", https_port);

    // 3) Carregar SSL_CERT e SSL_KEY
    const char *cert = get_ssl_cert();
    const char *key  = get_ssl_key();

    printf("[MASTER] A carregar certificado SSL: %s\n", cert);
    printf("[MASTER] A carregar chave privada: %s\n", key);

    global_ssl_ctx = ssl_server_init(cert, key);
    if (!global_ssl_ctx) {
        fprintf(stderr, "[MASTER] AVISO: Falha a inicializar SSL\n");
        fprintf(stderr, "[MASTER] HTTPS não estará disponível!\n");
        fprintf(stderr, "[MASTER] Verifique se cert.pem e key.pem existem.\n");
    } else {
        printf("[MASTER] SSL inicializado com sucesso!\n");
    }

    // 4) Lançar workers
    launch_workers(listen_http, listen_https);

    signal(SIGINT, sigint_handler);
    signal(SIGTERM, sigint_handler);

    printf("[MASTER] Servidor a correr. Prima CTRL+C para parar.\n");
    printf("[MASTER] ========================================\n");

    // 5) MASTER espera pelos workers
    while (1) {
        int status;
        pid_t p = wait(&status);
        if (p < 0) break;
        printf("[MASTER] Worker %d terminou\n", p);
    }

    printf("[MASTER] A limpar recursos...\n");

    close(listen_http);
    close(listen_https);

    logger_cleanup();
    cache_cleanup();

    if (global_ssl_ctx)
        ssl_server_cleanup(global_ssl_ctx);

    printf("[MASTER] Servidor terminado.\n");
    return 0;
}