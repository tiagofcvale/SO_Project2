#ifndef STATS_H
#define STATS_H

#include <semaphore.h>

typedef struct {
    long total_requests;
    long bytes_transferred;
    long status_200;
    long status_400;
    long status_403;
    long status_404;
    long status_500;
    int active_connections;
} server_stats_t;


void stats_init(server_stats_t *stats);
void stats_update(server_stats_t *stats, sem_t *mutex, int status_code, long bytes);
void stats_connection_start(server_stats_t *stats, sem_t *mutex);
void stats_connection_end(server_stats_t *stats, sem_t *mutex);
void stats_print(server_stats_t *stats, sem_t *mutex);

#endif