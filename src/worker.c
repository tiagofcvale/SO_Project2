#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
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

        // mandar o socket para uma thread deste worker
        thread_pool_add(&pool, client_socket);
        // a thread é que faz http_handle_request() e close()
    }
}