#include <stdio.h>
#include <fcntl.h>
#include <semaphore.h>
#include "semaphores.h"

int sem_init_ipc(ipc_semaphores_t *sems, int max_concurrent_accepts) {
    sem_unlink("/sem_ws_accept");
    sem_unlink("/sem_ws_stats");
    sem_unlink("/sem_ws_log");

    // Permite N workers aceitarem simultaneamente (controlo de carga)
    sems->sem_accept = sem_open("/sem_ws_accept", O_CREAT, 0666, max_concurrent_accepts);
    sems->sem_stats  = sem_open("/sem_ws_stats", O_CREAT, 0666, 1);
    sems->sem_log    = sem_open("/sem_ws_log",   O_CREAT, 0666, 1);

    if (sems->sem_accept == SEM_FAILED || 
        sems->sem_stats == SEM_FAILED ||
        sems->sem_log == SEM_FAILED) {
        perror("sem_init_ipc");
        return -1;
    }
    return 0;
}

void sem_cleanup_ipc(ipc_semaphores_t *sems) {
    if(sems->sem_accept != SEM_FAILED) sem_close(sems->sem_accept);
    if(sems->sem_stats != SEM_FAILED) sem_close(sems->sem_stats);
    if(sems->sem_log != SEM_FAILED) sem_close(sems->sem_log);

    sem_unlink("/sem_ws_accept");
    sem_unlink("/sem_ws_stats");
    sem_unlink("/sem_ws_log");
}