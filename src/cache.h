#ifndef CACHE_H
#define CACHE_H

#include <stddef.h>

void cache_init(size_t max_mb);
int cache_get(const char *path, char **data, size_t *size);
void cache_put(const char *path, const char *data, size_t size);

#endif