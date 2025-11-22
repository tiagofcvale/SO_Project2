#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>

#include "stats.h"

// Estatísticas simples (podem ser expandidas)
static long total_requests = 0;

static pthread_mutex_t stats_mutex = PTHREAD_MUTEX_INITIALIZER;


// ------------------------------------------------------------
// Inicializar módulo de estatísticas
// ------------------------------------------------------------
void stats_init(int max_workers) {
    (void)max_workers; // parâmetro não usado (mas pode ser útil no futuro)
    total_requests = 0;
}


// ------------------------------------------------------------
// Incrementar contador
// ------------------------------------------------------------
void stats_inc_requests(void) {
    pthread_mutex_lock(&stats_mutex);
    total_requests++;
    pthread_mutex_unlock(&stats_mutex);
}


// ------------------------------------------------------------
// Obter número total de pedidos servidos
// ------------------------------------------------------------
long stats_get_total_requests(void) {
    pthread_mutex_lock(&stats_mutex);
    long n = total_requests;
    pthread_mutex_unlock(&stats_mutex);
    return n;
}
