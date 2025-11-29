#ifndef SHARED_MEM_H
#define SHARED_MEM_H

#include "connection_queue.h"
#include "stats.h"

// Nome identificador da memória partilhada no sistema
#define SHM_NAME "/webserver_shm_v1"

typedef struct {
    connection_queue_t queue; // A fila circular de sockets (definida no connection_queue.h)
    server_stats_t stats;     // As estatísticas globais (definida no stats.h)
} shared_data_t;

// Função para criar/abrir a memória partilhada
shared_data_t* shm_create(void);

// Função para destruir/limpar a memória partilhada
void shm_destroy(shared_data_t* data);

#endif