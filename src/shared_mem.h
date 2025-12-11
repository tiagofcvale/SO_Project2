#ifndef SHARED_MEM_H
#define SHARED_MEM_H

#include "stats.h"

#define SHM_NAME "/webserver_shm_v1"

// A fila agora é de "tickets" não de sockets
typedef struct {
    int capacity;        // Capacidade máxima
    int active_accepts;  // Quantos workers estão em accept() agora
    int total_accepted;  // Total de conexões aceites (estatística)
} accept_control_t;

typedef struct {
    accept_control_t accept_ctrl;
    server_stats_t stats;
} shared_data_t;

shared_data_t* shm_create_master(void);
shared_data_t* shm_attach_worker(void);
void shm_destroy(shared_data_t* data);

#endif