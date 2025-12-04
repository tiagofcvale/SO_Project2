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
 * @brief Global pointer to shared memory, accessible by other modules.
 */
shared_data_t* shm_data = NULL;

/**
 * @brief Global structure with the IPC semaphores used by the worker.
 */
ipc_semaphores_t sems;

/**
 * @brief Main function of the worker process. Attaches to shared memory, opens semaphores,
 *        creates the thread pool, and accepts client connections, distributing them to threads.
 * @param listen_fd File descriptor of the listening socket (accepts new TCP connections).
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

    // create this worker's local thread pool
    thread_pool_t pool;
    thread_pool_init(&pool, get_threads_per_worker());

    printf("Worker %d iniciated.\n", getpid());

    while (1) {
        struct sockaddr_in client_addr;
        socklen_t len = sizeof(client_addr);

        // each worker does accept() directly
        int client_socket = accept(listen_fd,
                                   (struct sockaddr *)&client_addr,
                                   &len);

        if (client_socket < 0) {
            perror("worker accept");
            continue;
        }

        printf("Worker %d receive socket %d from %s:%d\n",
               getpid(),
               client_socket,
               inet_ntoa(client_addr.sin_addr),
               ntohs(client_addr.sin_port));

        // send the socket to a thread of this worker
        // If it fails (returns -1), the socket has already been closed by thread_pool_add
        thread_pool_add(&pool, client_socket);
    }
}