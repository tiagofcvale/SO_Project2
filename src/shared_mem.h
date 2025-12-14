#ifndef SHARED_MEM_H
#define SHARED_MEM_H

#include "stats.h"

#define SHM_NAME "/webserver_shm_v2"
#define CONN_QUEUE_SIZE 256  // Still used for semaphore initialization
#define MAX_WORKERS 32

// Shared data structure
typedef struct {
    server_stats_t stats;
    
    // Unix domain sockets for passing FDs to workers
    // [0] = master's end, [1] = worker's end
    int worker_sockets[MAX_WORKERS][2];
    int num_workers;
} shared_data_t;

shared_data_t* shm_create_master(void);
shared_data_t* shm_attach_worker(void);
void shm_destroy(shared_data_t* data);

#endif