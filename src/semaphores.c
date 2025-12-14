#include <stdio.h>
#include <fcntl.h>
#include <semaphore.h>
#include "semaphores.h"

int sem_init_ipc(ipc_semaphores_t *sems, int queue_capacity) {
    // Remove semáforos antigos
    sem_unlink("/sem_ws_queue_mutex");
    sem_unlink("/sem_ws_queue_empty");
    sem_unlink("/sem_ws_queue_full");
    sem_unlink("/sem_ws_stats");
    sem_unlink("/sem_ws_log");

    // Semáforos para a fila (PRODUTOR-CONSUMIDOR)
    sems->sem_queue_mutex = sem_open("/sem_ws_queue_mutex", O_CREAT, 0666, 1);
    sems->sem_queue_empty = sem_open("/sem_ws_queue_empty", O_CREAT, 0666, queue_capacity);
    sems->sem_queue_full  = sem_open("/sem_ws_queue_full",  O_CREAT, 0666, 0);
    
    // Semáforos para stats e logs
    sems->sem_stats = sem_open("/sem_ws_stats", O_CREAT, 0666, 1);
    sems->sem_log   = sem_open("/sem_ws_log",   O_CREAT, 0666, 1);

    if (sems->sem_queue_mutex == SEM_FAILED || 
        sems->sem_queue_empty == SEM_FAILED ||
        sems->sem_queue_full == SEM_FAILED ||
        sems->sem_stats == SEM_FAILED ||
        sems->sem_log == SEM_FAILED) {
        perror("sem_init_ipc");
        return -1;
    }
    
    printf("[SEM] Initialized semaphores (queue capacity: %d)\n", queue_capacity);
    return 0;
}

void sem_cleanup_ipc(ipc_semaphores_t *sems) {
    if(sems->sem_queue_mutex != SEM_FAILED) sem_close(sems->sem_queue_mutex);
    if(sems->sem_queue_empty != SEM_FAILED) sem_close(sems->sem_queue_empty);
    if(sems->sem_queue_full != SEM_FAILED) sem_close(sems->sem_queue_full);
    if(sems->sem_stats != SEM_FAILED) sem_close(sems->sem_stats);
    if(sems->sem_log != SEM_FAILED) sem_close(sems->sem_log);

    sem_unlink("/sem_ws_queue_mutex");
    sem_unlink("/sem_ws_queue_empty");
    sem_unlink("/sem_ws_queue_full");
    sem_unlink("/sem_ws_stats");
    sem_unlink("/sem_ws_log");
}