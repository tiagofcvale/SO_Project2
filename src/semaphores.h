#ifndef SEMAPHORES_H
#define SEMAPHORES_H

#include <semaphore.h>

typedef struct {
    // Semáforos para a fila de conexões (PRODUTOR-CONSUMIDOR)
    sem_t *sem_queue_mutex;   // Protege acesso à fila
    sem_t *sem_queue_empty;   // Conta slots vazios (produtor espera)
    sem_t *sem_queue_full;    // Conta slots cheios (consumidor espera)
    
    // Semáforos para estatísticas e logs
    sem_t *sem_stats;
    sem_t *sem_log;
} ipc_semaphores_t;

int sem_init_ipc(ipc_semaphores_t *sems, int queue_capacity);
void sem_cleanup_ipc(ipc_semaphores_t *sems);

#endif