#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>

#include "cache.h"

static cache_entry_t *cache_table = NULL;
static size_t cache_capacity = 0;
static size_t cache_count = 0;

static pthread_mutex_t cache_mutex = PTHREAD_MUTEX_INITIALIZER;


// ------------------------------------------------------------
// hash simples para mapear nomes → índices
// ------------------------------------------------------------
static size_t hash_path(const char *path) {
    unsigned long h = 5381;
    int c;
    while ((c = *path++))
        h = ((h << 5) + h) + c;
    return h % cache_capacity;
}


// ------------------------------------------------------------
// Inicializar cache com capacidade em MB
// ------------------------------------------------------------
void cache_init(int mb) {

    size_t bytes = (size_t)mb * 1024 * 1024;

    // cada entrada terá ~1 KB (aprox)
    cache_capacity = bytes / sizeof(cache_entry_t);
    if (cache_capacity < 64)
        cache_capacity = 64;

    cache_table = calloc(cache_capacity, sizeof(cache_entry_t));
    cache_count = 0;

    printf("Cache inicializada com %zu entradas (~%d MB)\n",
           cache_capacity, mb);
}


// ------------------------------------------------------------
// Verificar se ficheiro está em cache
// ------------------------------------------------------------
int cache_get(const char *path, char **data, size_t *size) {

    pthread_mutex_lock(&cache_mutex);

    size_t idx = hash_path(path);
    cache_entry_t *e = &cache_table[idx];

    if (!e->valid) {
        pthread_mutex_unlock(&cache_mutex);
        return 0;
    }

    if (strcmp(e->path, path) != 0) {
        pthread_mutex_unlock(&cache_mutex);
        return 0;
    }

    *data = e->data;
    *size = e->size;

    pthread_mutex_unlock(&cache_mutex);
    return 1;
}


// ------------------------------------------------------------
// Inserir ficheiro na cache
// ------------------------------------------------------------
void cache_put(const char *path, char *data, size_t size) {

    pthread_mutex_lock(&cache_mutex);

    size_t idx = hash_path(path);
    cache_entry_t *e = &cache_table[idx];

    // limpar entrada antiga
    if (e->valid && e->data)
        free(e->data);

    // guardar
    e->data = malloc(size);
    memcpy(e->data, data, size);
    e->size = size;
    strncpy(e->path, path, sizeof(e->path)-1);
    e->path[sizeof(e->path)-1] = '\0';

    e->valid = 1;

    pthread_mutex_unlock(&cache_mutex);
}

