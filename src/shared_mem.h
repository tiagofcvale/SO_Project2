#ifndef SHARED_MEM_H
#define SHARED_MEM_H

#include "stats.h"

#define SHM_NAME "/webserver_shm_v1"

// The queue is now of "tickets" not sockets
typedef struct {
    int capacity;        // Maximum capacity
    int active_accepts;  // How many workers are in accept() now
    int total_accepted;  // Total accepted connections (statistic)
} accept_control_t;

typedef struct {
    accept_control_t accept_ctrl;
    server_stats_t stats;
} shared_data_t;

shared_data_t* shm_create_master(void);
shared_data_t* shm_attach_worker(void);
void shm_destroy(shared_data_t* data);

#endif