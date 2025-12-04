#include <stdio.h>
#include <semaphore.h>
#include "stats.h"

/**
 * @brief Initializes the statistics structure to zero.
 * @param stats Pointer to the statistics structure to initialize.
 */
void stats_init(server_stats_t *stats) {
    if (stats) {
        stats->total_requests = 0;
        stats->bytes_transferred = 0;
        stats->status_200 = 0;
        stats->status_400 = 0;
        stats->status_403 = 0;
        stats->status_404 = 0;
        stats->status_500 = 0;
        stats->active_connections = 0;
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

    // Enter critical section (Blocks other processes)
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

    // Exit critical section (Releases for other processes)
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

    // Quick copy to avoid blocking the server while printing
    sem_wait(mutex);
    snapshot = *stats;
    sem_post(mutex);

    printf("\n=== Statistics ===\n");
    printf("Total Requests:   %ld\n", snapshot.total_requests);
    printf("Bytes:            %ld\n", snapshot.bytes_transferred);
    printf("Active:           %d\n", snapshot.active_connections);
    printf("\n--- HTTP Status Codes ---\n");
    printf("200 OK:           %ld\n", snapshot.status_200);
    printf("400 Bad Request:  %ld\n", snapshot.status_400);
    printf("403 Forbidden:    %ld\n", snapshot.status_403);
    printf("404 Not Found:    %ld\n", snapshot.status_404);
    printf("500 Server Error: %ld\n", snapshot.status_500);
    printf("========================\n");
}