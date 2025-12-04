#ifndef SHARED_MEM_H
#define SHARED_MEM_H

#include "connection_queue.h"
#include "stats.h"

// Name identifier of the shared memory in the system
#define SHM_NAME "/webserver_shm_v1"

typedef struct {
    connection_queue_t queue; // The circular socket queue (defined in connection_queue.h)
    server_stats_t stats;     // The global statistics (defined in stats.h)
} shared_data_t;

// Function to create/open the shared memory
shared_data_t* shm_create(void);

// Function to destroy/clean the shared memory
void shm_destroy(shared_data_t* data);

#endif