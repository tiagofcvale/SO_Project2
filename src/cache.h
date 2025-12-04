#ifndef CACHE_H
#define CACHE_H

#include <stddef.h>

// ------------------------------------------------------------
// Internal structure of each cache entry
// ------------------------------------------------------------
typedef struct {
    char path[1024];   // file path
    char *data;        // content in RAM
    size_t size;       // file size
    int valid;         // 1 if valid, 0 if empty
} cache_entry_t;


// ------------------------------------------------------------
// Cache API
// ------------------------------------------------------------

// Initialize cache with X MB
void cache_init(int mb);

// Get file from cache (returns 1 if exists)
int cache_get(const char *path, char **data, size_t *size);

// Put file in cache
void cache_put(const char *path, char *data, size_t size);

// Clean up cache
void cache_cleanup(void);

#endif
