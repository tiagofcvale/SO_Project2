#include "cache.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <time.h>

#define MAX_CACHE_ENTRIES 1024

typedef struct {
    char path[1024];
    char *data;
    size_t size;
    time_t last_used;
} cache_entry_t;

static cache_entry_t cache[MAX_CACHE_ENTRIES];
static size_t current_size = 0;
static size_t max_size = 0;

void cache_init(size_t max_mb) {
    max_size = max_mb * 1024 * 1024;
    current_size = 0;
    memset(cache, 0, sizeof(cache));
}

int cache_get(const char *path, char **data, size_t *size) {
    for (int i = 0; i < MAX_CACHE_ENTRIES; i++) {
        if (strcmp(cache[i].path, path) == 0) {
            cache[i].last_used = time(NULL);
            *data = cache[i].data;
            *size = cache[i].size;
            return 1;  // FOUND
        }
    }
    return 0; // NOT FOUND
}

static void evict_lru() {
    int lru = -1;
    time_t oldest = time(NULL);

    for (int i = 0; i < MAX_CACHE_ENTRIES; i++) {
        if (cache[i].data != NULL && cache[i].last_used <= oldest) {
            oldest = cache[i].last_used;
            lru = i;
        }
    }

    if (lru != -1) {
        current_size -= cache[lru].size;
        free(cache[lru].data);
        cache[lru].data = NULL;
        cache[lru].size = 0;
        cache[lru].path[0] = '\0';
    }
}

void cache_put(const char *path, const char *data, size_t size) {

    if (size > max_size) return; // Never save big files

    // free space (if necessary)
    while (current_size + size > max_size) {
        evict_lru();
    }

    for (int i = 0; i < MAX_CACHE_ENTRIES; i++) {
        if (cache[i].data == NULL) {
            strcpy(cache[i].path, path);

            cache[i].data = malloc(size);
            memcpy(cache[i].data, data, size);

            cache[i].size = size;
            cache[i].last_used = time(NULL);

            current_size += size;
            return;
        }
    }
}
