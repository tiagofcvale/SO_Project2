#define _POSIX_C_SOURCE 200809L

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include "cache.h"

static cache_entry_t *cache_table = NULL;
static size_t cache_capacity = 0;
static size_t cache_count = 0;

// Reader-Writer lock instead of simple mutex
static pthread_rwlock_t cache_rwlock = PTHREAD_RWLOCK_INITIALIZER;

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

    // Initialize the reader-writer lock
    pthread_rwlock_init(&cache_rwlock, NULL);

    printf("Cache inicializada com %zu entradas (~%d MB) [RW-Lock]\n",
           cache_capacity, mb);
}

// ------------------------------------------------------------
// Verificar se ficheiro está em cache (READ operation)
// ------------------------------------------------------------
int cache_get(const char *path, char **data, size_t *size) {
    // Acquire READ lock - multiple threads can read simultaneously
    pthread_rwlock_rdlock(&cache_rwlock);

    size_t idx = hash_path(path);
    cache_entry_t *e = &cache_table[idx];

    if (!e->valid) {
        pthread_rwlock_unlock(&cache_rwlock);
        return 0;
    }

    if (strcmp(e->path, path) != 0) {
        pthread_rwlock_unlock(&cache_rwlock);
        return 0;
    }

    *data = e->data;
    *size = e->size;

    pthread_rwlock_unlock(&cache_rwlock);
    return 1;
}

// ------------------------------------------------------------
// Inserir ficheiro na cache (WRITE operation)
// ------------------------------------------------------------
void cache_put(const char *path, char *data, size_t size) {
    // Acquire WRITE lock - exclusive access for writing
    pthread_rwlock_wrlock(&cache_rwlock);

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

    pthread_rwlock_unlock(&cache_rwlock);
}

// ------------------------------------------------------------
// Cleanup cache (optional but good practice)
// ------------------------------------------------------------
void cache_cleanup(void) {
    pthread_rwlock_wrlock(&cache_rwlock);

    if (cache_table) {
        for (size_t i = 0; i < cache_capacity; i++) {
            if (cache_table[i].valid && cache_table[i].data) {
                free(cache_table[i].data);
            }
        }
        free(cache_table);
        cache_table = NULL;
    }

    pthread_rwlock_unlock(&cache_rwlock);
    pthread_rwlock_destroy(&cache_rwlock);
}