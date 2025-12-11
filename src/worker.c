#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <errno.h>
#include <time.h>
#include <string.h>

#include "config.h"
#include "worker.h"
#include "thread_pool.h"
#include "shared_mem.h"
#include "semaphores.h"

shared_data_t* shm_data = NULL;
ipc_semaphores_t sems;

void worker_main(int listen_fd) {
    // 1. Attach to shared memory (NÃO cria)
    shm_data = shm_attach_worker();
    if (!shm_data) {
        fprintf(stderr, "Worker %d: Failed to attach SHM\n", getpid());
        exit(1);
    }

    // 2. Abre semáforos existentes
    sems.sem_accept = sem_open("/sem_ws_accept", 0);
    sems.sem_stats  = sem_open("/sem_ws_stats", 0);
    sems.sem_log    = sem_open("/sem_ws_log", 0);

    if (sems.sem_accept == SEM_FAILED || 
        sems.sem_stats == SEM_FAILED ||
        sems.sem_log == SEM_FAILED) {
        perror("Worker: sem_open failed");
        exit(1);
    }

    // 3. Cria thread pool
    thread_pool_t pool;
    thread_pool_init(&pool, get_threads_per_worker());
    
    printf("Worker %d initialized with %d threads\n", 
           getpid(), get_threads_per_worker());

    // 4. LOOP PRINCIPAL: Worker faz accept() controlado
    while (1) {
        // PRODUTOR-CONSUMIDOR: Aguarda "autorização" para aceitar
        // (controla carga do sistema)
        struct timespec ts;
        clock_gettime(CLOCK_REALTIME, &ts);
        ts.tv_sec += 5;  // Timeout de 5s
        
        if (sem_timedwait(sems.sem_accept, &ts) < 0) {
            if (errno == ETIMEDOUT) {
                continue;  // Tenta novamente
            }
            perror("sem_timedwait");
            continue;
        }

        // CRITICAL: Incrementa contador de accepts ativos
        sem_wait(sems.sem_stats);
        shm_data->accept_ctrl.active_accepts++;
        sem_post(sems.sem_stats);

        // ACCEPT: Agora pode aceitar uma conexão
        struct sockaddr_in client_addr;
        socklen_t len = sizeof(client_addr);
        
        int client_socket = accept(listen_fd, 
                                   (struct sockaddr *)&client_addr, 
                                   &len);

        // CRITICAL: Decrementa contador
        sem_wait(sems.sem_stats);
        shm_data->accept_ctrl.active_accepts--;
        if (client_socket >= 0) {
            shm_data->accept_ctrl.total_accepted++;
        }
        sem_post(sems.sem_stats);

        // Liberta o "ticket" para outro worker
        sem_post(sems.sem_accept);

        if (client_socket < 0) {
            if (errno == EINTR || errno == EAGAIN) {
                continue;
            }
            perror("accept");
            continue;
        }

        printf("Worker %d: Accepted socket %d from %s:%d\n",
               getpid(), client_socket,
               inet_ntoa(client_addr.sin_addr),
               ntohs(client_addr.sin_port));

        // Envia para thread pool processar
        if (thread_pool_add(&pool, client_socket) < 0) {
            // Thread pool cheio - fecha socket
            const char *resp = "HTTP/1.1 503 Service Unavailable\r\n"
                             "Content-Type: text/plain\r\n"
                             "Connection: close\r\n\r\n"
                             "Server overloaded\n";
            send(client_socket, resp, strlen(resp), 0);
            close(client_socket);
        }
    }
}