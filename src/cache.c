#define _POSIX_C_SOURCE 200809L

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include "cache.h"
#include "shared_mem.h"
#include "semaphores.h"
#include "global.h"

static cache_entry_t *cache_table = NULL;
static size_t cache_capacity = 0;
static size_t cache_count = 0;

// Reader-Writer lock instead of simple mutex
static pthread_rwlock_t cache_rwlock = PTHREAD_RWLOCK_INITIALIZER;


/**
 * @brief Computes a simple hash for a file path.
 * @param path File path.
 * @return Hash index for the cache table.
 */
static size_t hash_path(const char *path) {
    unsigned long h = 5381;
    int c;
    while ((c = *path++))
        h = ((h << 5) + h) + c;
    return h % cache_capacity;
}


/**
 * @brief Initializes the in-memory cache with the given capacity (in MB).
 * @param mb Cache size in megabytes.
 */
void cache_init(int mb) {
    size_t bytes = (size_t)mb * 1024 * 1024;
    
    // each entry will have aproximately 1 KB 
    cache_capacity = bytes / sizeof(cache_entry_t);
    if (cache_capacity < 64)
        cache_capacity = 64;

    cache_table = calloc(cache_capacity, sizeof(cache_entry_t));
    cache_count = 0;

    // Initialize the reader-writer lock
    pthread_rwlock_init(&cache_rwlock, NULL);

    printf("Cache initialized with %zu entries (~%d MB) [RW-Lock]\n",
           cache_capacity, mb);
}


/**
 * @brief Checks if a file is in the cache and, if so, returns the data.
 * @param path Path of the file to look for.
 * @param data Pointer to where the pointer to the data will be stored.
 * @param size Pointer to where the size of the data will be stored.
 * @return 1 if found, 0 otherwise.
 */
int cache_get(const char *path, char **data, size_t *size) {
    pthread_rwlock_rdlock(&cache_rwlock);

    size_t idx = hash_path(path);
    cache_entry_t *e = &cache_table[idx];

    if (!e->valid || strcmp(e->path, path) != 0) {
        pthread_rwlock_unlock(&cache_rwlock);
        
        // CACHE MISS
        if (shm_data && sems.sem_stats) {
            sem_wait(sems.sem_stats);
            shm_data->stats.cache_misses++;
            sem_post(sems.sem_stats);
        }
        
        return 0;
    }

    *data = e->data;
    *size = e->size;
    
    // CACHE HIT
    if (shm_data && sems.sem_stats) {
        sem_wait(sems.sem_stats);
        shm_data->stats.cache_hits++;
        sem_post(sems.sem_stats);
    }

    pthread_rwlock_unlock(&cache_rwlock);
    return 1;
}


/**
 * @brief Inserts or updates a file in the cache.
 * @param path File path.
 * @param data Pointer to the data to store.
 * @param size Size of the data.
 */
void cache_put(const char *path, char *data, size_t size) {
    pthread_rwlock_wrlock(&cache_rwlock);

    size_t idx = hash_path(path);
    cache_entry_t *e = &cache_table[idx];

    // Se já existe entrada válida com path diferente, não sobrescreve
    if (e->valid && strcmp(e->path, path) != 0) {
        printf("[Cache] Collision at index %zu: '%s' vs '%s' - skipping\n",
               idx, e->path, path);
        pthread_rwlock_unlock(&cache_rwlock);
        return;
    }

    // Limpa entrada antiga
    if (e->valid && e->data)
        free(e->data);

    // Armazena nova entrada
    e->data = malloc(size);
    memcpy(e->data, data, size);
    e->size = size;
    strncpy(e->path, path, sizeof(e->path)-1);
    e->path[sizeof(e->path)-1] = '\0';
    e->valid = 1;

    pthread_rwlock_unlock(&cache_rwlock);
}


/**
 * @brief Frees all memory associated with the cache and destroys the lock.
 */
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