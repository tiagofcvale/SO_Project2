#include <stdio.h>
#include <fcntl.h>
#include <semaphore.h>
#include <errno.h>
#include "semaphores.h"

/**
 * @brief Initializes and creates the named POSIX semaphores used by the server.
 *        Removes old semaphores, creates new ones, and associates them with the provided structure.
 * @param sems Pointer to the structure where the semaphore descriptors are stored.
 * @param queue_size Request queue size (initial value for the empty semaphore).
 * @return 0 on success, -1 on error.
 */
int sem_init_ipc(ipc_semaphores_t *sems, int queue_size) {
    // Preventive cleanup from previous runs
    sem_unlink("/sem_ws_empty");
    sem_unlink("/sem_ws_full");
    sem_unlink("/sem_ws_mutex");
    sem_unlink("/sem_ws_stats");
    sem_unlink("/sem_ws_log");

    // Open semaphores with O_CREAT
    // 0666 permissions for read/write
    
    // sem_empty starts with queue_size (queue completely empty)
    sems->sem_empty = sem_open("/sem_ws_empty", O_CREAT, 0666, queue_size);
    
    // sem_full starts with 0 (no items in the queue)
    sems->sem_full  = sem_open("/sem_ws_full",  O_CREAT, 0666, 0);
    
    // Mutexes start with 1 (free)
    sems->sem_mutex = sem_open("/sem_ws_mutex", O_CREAT, 0666, 1);
    sems->sem_stats = sem_open("/sem_ws_stats", O_CREAT, 0666, 1);
    sems->sem_log   = sem_open("/sem_ws_log",   O_CREAT, 0666, 1);

    // Check for errors
    if (sems->sem_empty == SEM_FAILED || sems->sem_full == SEM_FAILED || 
        sems->sem_mutex == SEM_FAILED || sems->sem_stats == SEM_FAILED ||
        sems->sem_log == SEM_FAILED) {
        perror("sem_init_ipc failed");
        return -1;
    }
    return 0;
}

/**
 * @brief Closes and removes the named POSIX semaphores used by the server.
 *        Closes the descriptors and unlinks the semaphores from the operating system.
 * @param sems Pointer to the structure with the semaphore descriptors.
 */
void sem_cleanup_ipc(ipc_semaphores_t *sems) {
    // Close descriptors
    if(sems->sem_empty != SEM_FAILED) sem_close(sems->sem_empty);
    if(sems->sem_full  != SEM_FAILED) sem_close(sems->sem_full);
    if(sems->sem_mutex != SEM_FAILED) sem_close(sems->sem_mutex);
    if(sems->sem_stats != SEM_FAILED) sem_close(sems->sem_stats);
    if(sems->sem_log   != SEM_FAILED) sem_close(sems->sem_log);

    // Remove from the operating system (unlink)
    sem_unlink("/sem_ws_empty");
    sem_unlink("/sem_ws_full");
    sem_unlink("/sem_ws_mutex");
    sem_unlink("/sem_ws_stats");
    sem_unlink("/sem_ws_log");
}