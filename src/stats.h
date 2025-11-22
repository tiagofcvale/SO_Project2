#ifndef STATS_H
#define STATS_H

// Inicializar o módulo de estatísticas
void stats_init(int max_workers);

// Incrementar total de pedidos
void stats_inc_requests(void);

// Obter número total de pedidos servidos
long stats_get_total_requests(void);

#endif
