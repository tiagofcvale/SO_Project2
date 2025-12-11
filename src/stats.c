#include <stdio.h>
#include <semaphore.h>
#include <string.h>
#include "stats.h"

/**
 * @brief Initializes the statistics structure to zero.
 * @param stats Pointer to the statistics structure to initialize.
 */
void stats_init(server_stats_t *stats) {
    if (stats) {
        memset(stats, 0, sizeof(server_stats_t));
    }
}

/**
 * @brief Safely updates the statistics counters (thread-safe and process-safe).
 * @param stats Pointer to the statistics structure.
 * @param mutex Semaphore for critical section protection.
 * @param status_code HTTP status code of the request.
 * @param bytes Number of bytes transferred in this request.
 */

void stats_update(server_stats_t *stats, sem_t *mutex, int status_code, long bytes) {
    if (!stats || !mutex) return;

    sem_wait(mutex);
    stats->total_requests++;
    stats->bytes_transferred += bytes;

    switch (status_code) {
        case 200: stats->status_200++; break;
        case 400: stats->status_400++; break;
        case 403: stats->status_403++; break;
        case 404: stats->status_404++; break;
        case 500: stats->status_500++; break;
    }
    sem_post(mutex);
}

/**
 * @brief Safely increments the number of active connections.
 * @param stats Pointer to the statistics structure.
 * @param mutex Semaphore for critical section protection.
 */
void stats_connection_start(server_stats_t *stats, sem_t *mutex) {
    if (!stats || !mutex) return;
    sem_wait(mutex);
    stats->active_connections++;
    sem_post(mutex);
}
/**
 * @brief Safely decrements the number of active connections.
 * @param stats Pointer to the statistics structure.
 * @param mutex Semaphore for critical section protection.
 */
void stats_connection_end(server_stats_t *stats, sem_t *mutex) {
    if (!stats || !mutex) return;
    sem_wait(mutex);
    stats->active_connections--;
    sem_post(mutex);
}

/**
 * @brief Safely prints the current statistics.
 * @param stats Pointer to the statistics structure.
 * @param mutex Semaphore for critical section protection.
 */
void stats_print(server_stats_t *stats, sem_t *mutex) {
    if (!stats || !mutex) return;
    
    server_stats_t snapshot;
    sem_wait(mutex);
    snapshot = *stats;
    sem_post(mutex);

    printf("\n======================================\n");
    printf("         SERVER STATISTICS              \n");
    printf("========================================\n");
    printf(" Total Requests:      %10ld       \n", snapshot.total_requests);
    printf(" Bytes Transferred:   %10ld       \n", snapshot.bytes_transferred);
    printf(" Active Connections:  %10d       \n", snapshot.active_connections);
    printf("========================================\n");
    printf(" HTTP Status Codes:                     \n");
    printf("   200 OK:            %10ld       \n", snapshot.status_200);
    printf("   400 Bad Request:   %10ld       \n", snapshot.status_400);
    printf("   403 Forbidden:     %10ld       \n", snapshot.status_403);
    printf("   404 Not Found:     %10ld       \n", snapshot.status_404);
    printf("   500 Server Error:  %10ld       \n", snapshot.status_500);
    printf("========================================\n");
    printf(" Cache Statistics:                      \n");
    printf("   Hits:              %10ld       \n", snapshot.cache_hits);
    printf("   Misses:            %10ld       \n", snapshot.cache_misses);
    
    if ((snapshot.cache_hits + snapshot.cache_misses) > 0) {
        double hit_rate = (double)snapshot.cache_hits / 
                         (snapshot.cache_hits + snapshot.cache_misses) * 100.0;
        printf("   Hit Rate:          %9.2f%%      \n", hit_rate);
    }
    
    printf("========================================\n");
}