#ifndef SEMAPHORES_H
#define SEMAPHORES_H

#include <semaphore.h>

typedef struct {
    sem_t *sem_accept;  // Controla quantos workers podem aceitar
    sem_t *sem_stats;   // Protege estatísticas
    sem_t *sem_log;     // Protege logs
} ipc_semaphores_t;

int sem_init_ipc(ipc_semaphores_t *sems, int max_concurrent_accepts);
void sem_cleanup_ipc(ipc_semaphores_t *sems);

#endif