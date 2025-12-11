#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <string.h>
#include <signal.h>
#include <fcntl.h>

#include "master.h"
#include "config.h"
#include "logger.h"
#include "worker.h"
#include "shared_mem.h"
#include "semaphores.h"

static int server_socket = -1;
static shared_data_t *shm_data = NULL;
static ipc_semaphores_t sems;
static pid_t *worker_pids = NULL;

void handle_sigint(int sig) {
    static int already_cleaned = 0;
    if (already_cleaned) return;
    already_cleaned = 1;
    (void)sig;
    
    printf("\n[Master] Received SIGINT. Shutting down...\n");

    int n = get_num_workers();
    if (worker_pids) {
        for (int i = 0; i < n; i++) {
            if (worker_pids[i] > 0) {
                kill(worker_pids[i], SIGTERM);
            }
        }
        free(worker_pids);
    }

    if (server_socket >= 0) close(server_socket);
    
    shm_destroy(shm_data);
    sem_cleanup_ipc(&sems);
    logger_cleanup();
    
    printf("[Master] Shutdown complete.\n");
    exit(0);
}

int master_start(void) {
    signal(SIGINT, handle_sigint);

    // 1. Cria SHM (Master é o criador)
    shm_data = shm_create_master();
    if (!shm_data) {
        fprintf(stderr, "Failed to create shared memory\n");
        return -1;
    }
    
    // Inicializa estruturas
    stats_init(&shm_data->stats);
    shm_data->accept_ctrl.capacity = get_max_queue_size();
    shm_data->accept_ctrl.active_accepts = 0;
    shm_data->accept_ctrl.total_accepted = 0;

    // 2. Cria semáforos
    // O semáforo de accept limita quantos workers podem aceitar simultaneamente
    int max_concurrent = get_num_workers();  // Todos podem aceitar
    if (sem_init_ipc(&sems, max_concurrent) < 0) {
        shm_destroy(shm_data);
        return -1;
    }

    // 3. Cria socket
    struct sockaddr_in server_addr;
    int opt = 1;

    server_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (server_socket < 0) {
        perror("socket");
        return -1;
    }
    
    setsockopt(server_socket, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(get_server_port());

    if (bind(server_socket, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("bind");
        return -1;
    }
    
    if (listen(server_socket, 128) < 0) {
        perror("listen");
        return -1;
    }

    printf("[Master] Listening on port %d\n", get_server_port());

    // 4. Cria Workers (eles herdam o server_socket)
    int n_workers = get_num_workers();
    worker_pids = malloc(sizeof(pid_t) * n_workers);

    for (int i = 0; i < n_workers; i++) {
        pid_t pid = fork();
        if (pid == 0) {
            // Worker
            setpgid(0, 0);
            signal(SIGINT, SIG_IGN);
            worker_main(server_socket);  // Worker recebe o listen socket
            exit(0);
        } else if (pid > 0) {
            worker_pids[i] = pid;
            printf("[Master] Worker %d created (PID %d)\n", i+1, pid);
        } else {
            perror("fork");
        }
    }

    // 5. Master fica a monitorizar
    printf("[Master] Server running (CTRL+C to stop)\n\n");
    
    while (1) {
        sleep(15);
        stats_print(&shm_data->stats, sems.sem_stats);
    }

    return 0;
}

int master_stop(void) {
    raise(SIGINT);
    return 0;
}