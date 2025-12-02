#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>
#include <signal.h>
#include <errno.h>
#include <fcntl.h>
#include <time.h>

#include "master.h"
#include "config.h"
#include "logger.h"
#include "worker.h"
#include "shared_mem.h"
#include "semaphores.h"

// Variáveis globais para limpeza no signal handler
static int server_socket = -1;
static shared_data_t *shm_data = NULL;
static ipc_semaphores_t sems;
static pid_t *worker_pids = NULL;


/**
 * @brief Envia uma resposta HTTP 503 e fecha o socket, usado quando a fila está cheia.
 * @param fd Descritor de socket do cliente.
 */
void send_503_and_close(int fd) {
    const char *resp = "HTTP/1.1 503 Service Unavailable\r\n"
                       "Content-Type: text/plain\r\n"
                       "Connection: close\r\n\r\n"
                       "Server too busy, try again later.\n";
    send(fd, resp, strlen(resp), 0);
    close(fd);
}

/**
 * @brief Handler para o sinal SIGINT (CTRL+C). Limpa recursos IPC, termina workers e encerra o servidor.
 * @param sig Número do sinal recebido (não usado).
 */
void handle_sigint(int sig) {
    (void)sig;
    printf("\n[Master] Recebido SIGINT. A encerrar servidor...\n");

    // 1. Matar processos workers
    int n = get_num_workers();
    if (worker_pids) {
        for (int i = 0; i < n; i++) {
            if (worker_pids[i] > 0) {
                kill(worker_pids[i], SIGTERM);
            }
        }
        free(worker_pids);
    }

    // 2. Fechar socket principal
    if (server_socket >= 0) {
        close(server_socket);
    }

    // 3. Limpar IPC (CRUCIAL: shm_unlink e sem_unlink)
    shm_destroy(shm_data);
    sem_cleanup_ipc(&sems);

    logger_cleanup();
    printf("[Master] Limpeza concluída. Adeus.\n");
    exit(0);
}


/**
 * @brief Função principal do processo master. Inicializa IPC, socket, cria workers e monitoriza o servidor.
 * @return 0 em caso de sucesso, -1 em caso de erro.
 */
int master_start(void) {
    signal(SIGINT, handle_sigint);

    // 1. Inicializar IPC
    shm_data = shm_create();
    if (!shm_data) return -1;
    
    // Inicializar Stats e Fila
    stats_init(&shm_data->stats);
    shm_data->queue.front = 0;
    shm_data->queue.rear = 0;
    shm_data->queue.count = 0;

    // Inicializar Semáforos
    if (sem_init_ipc(&sems, get_max_queue_size()) < 0) {
        shm_destroy(shm_data);
        return -1;
    }

    // 2. Criar Socket
    struct sockaddr_in server_addr;
    int opt = 1;

    server_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (server_socket < 0) { perror("socket"); return -1; }
    setsockopt(server_socket, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(get_server_port());

    if (bind(server_socket, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("bind"); return -1;
    }
    if (listen(server_socket, 128) < 0) {
        perror("listen"); return -1;
    }

    printf("[Master] A ouvir na porta %d. A lançar workers...\n", get_server_port());

    // 3. Criar Workers
    int n_workers = get_num_workers();
    worker_pids = malloc(sizeof(pid_t) * n_workers);

    for (int i = 0; i < n_workers; i++) {
        pid_t pid = fork();
        if (pid == 0) {
            // Worker HERDA o server_socket e vai usá-lo
            worker_main(server_socket); 
            exit(0);
        } else {
            worker_pids[i] = pid;
        }
    }

    // 4. Mestre em modo de Monitorização
    // O Mestre já não faz accept, apenas monitoriza e imprime stats
    printf("[Master] Servidor Ativo. (CTRL+C para sair)\n");
    
    while (1) {
        sleep(5); // Imprimir stats a cada 5 segundos
        stats_print(&shm_data->stats, sems.sem_stats);
    }

    return 0;
}

/**
 * @brief Para o servidor de forma ordenada (função opcional para interface)
 * @return 0 em sucesso
 */
int master_stop(void) {
    // Trigger do shutdown via sinal
    raise(SIGINT);
    return 0;
}