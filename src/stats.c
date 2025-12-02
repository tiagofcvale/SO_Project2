#include <stdio.h>
#include <semaphore.h>
#include "stats.h"

/**
 * @brief Inicializa a estrutura de estatísticas a zero.
 * @param stats Ponteiro para a estrutura de estatísticas a inicializar.
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
 * @brief Atualiza os contadores de estatísticas de forma segura (thread-safe e process-safe).
 * @param stats Ponteiro para a estrutura de estatísticas.
 * @param mutex Semáforo para proteção da secção crítica.
 * @param status_code Código de estado HTTP do pedido.
 * @param bytes Número de bytes transferidos neste pedido.
 */

void stats_update(server_stats_t *stats, sem_t *mutex, int status_code, long bytes) {
    if (!stats || !mutex) return;

    // Entrar na secção crítica (Bloqueia outros processos)
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

    // Sair da secção crítica (Liberta para outros processos)
    sem_post(mutex);
}

/**
 * @brief Incrementa o número de conexões ativas de forma segura.
 * @param stats Ponteiro para a estrutura de estatísticas.
 * @param mutex Semáforo para proteção da secção crítica.
 */
void stats_connection_start(server_stats_t *stats, sem_t *mutex) {
    if (!stats || !mutex) return;
    sem_wait(mutex);
    stats->active_connections++;
    sem_post(mutex);
}

/**
 * @brief Decrementa o número de conexões ativas de forma segura.
 * @param stats Ponteiro para a estrutura de estatísticas.
 * @param mutex Semáforo para proteção da secção crítica.
 */
void stats_connection_end(server_stats_t *stats, sem_t *mutex) {
    if (!stats || !mutex) return;
    sem_wait(mutex);
    stats->active_connections--;
    sem_post(mutex);
}

/**
 * @brief Imprime as estatísticas atuais de forma segura.
 * @param stats Ponteiro para a estrutura de estatísticas.
 * @param mutex Semáforo para proteção da secção crítica.
 */
void stats_print(server_stats_t *stats, sem_t *mutex) {
    if (!stats || !mutex) return;
    
    server_stats_t snapshot;

    // Copia rápida para evitar bloquear o servidor enquanto imprime
    sem_wait(mutex);
    snapshot = *stats;
    sem_post(mutex);

    printf("\n=== Estatísticas ===\n");
    printf("Pedidos Totais: %ld\n", snapshot.total_requests);
    printf("Bytes:          %ld\n", snapshot.bytes_transferred);
    printf("Ativos:         %d\n", snapshot.active_connections);
    printf("====================\n");
}