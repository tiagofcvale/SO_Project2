#ifndef SEMAPHORES_H
#define SEMAPHORES_H

#include <semaphore.h>

typedef struct {
    sem_t *sem_empty;  
    sem_t *sem_full;   
    sem_t *sem_mutex;  
    sem_t *sem_stats;  
    sem_t *sem_log;    
} ipc_semaphores_t;

// Initializes the named semaphores
int sem_init_ipc(ipc_semaphores_t *sems, int queue_size);

// Closes and removes the semaphores from the system
void sem_cleanup_ipc(ipc_semaphores_t *sems);

#endif