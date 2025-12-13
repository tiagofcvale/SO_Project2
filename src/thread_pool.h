#ifndef THREAD_POOL_H
#define THREAD_POOL_H

#include <pthread.h>
#include "worker.h"  // For connection_t

#define WORKER_QUEUE_SIZE 128

typedef struct {
    connection_t* connections[WORKER_QUEUE_SIZE];  // Changed from int sockets[] to connection_t*
    int front;
    int rear;
    int count;
    pthread_mutex_t mutex;
    pthread_cond_t cond_non_empty;
    pthread_cond_t cond_non_full;
} thread_pool_queue_t;

typedef struct {
    pthread_t *threads;
    int thread_count;
    thread_pool_queue_t queue;
} thread_pool_t;

void thread_pool_init(thread_pool_t *pool, int n);
void thread_pool_add(thread_pool_t *pool, connection_t* conn);  // Changed from int to connection_t*

#endif