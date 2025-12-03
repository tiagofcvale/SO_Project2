#ifndef CACHE_H
#define CACHE_H

#include <stddef.h>

// ------------------------------------------------------------
// Estrutura interna de cada entrada de cache
// ------------------------------------------------------------
typedef struct {
    char path[1024];   // caminho do ficheiro
    char *data;        // conteúdo em RAM
    size_t size;       // tamanho do ficheiro
    int valid;         // 1 se válido, 0 se vazio
} cache_entry_t;


// ------------------------------------------------------------
// API da cache
// ------------------------------------------------------------

// Inicializar cache com X MB
void cache_init(int mb);

// Obter ficheiro da cache (retorna 1 se existir)
int cache_get(const char *path, char **data, size_t *size);

// Colocar ficheiro na cache
void cache_put(const char *path, char *data, size_t size);

// Clean à cache
void cache_cleanup(void);

#endif
