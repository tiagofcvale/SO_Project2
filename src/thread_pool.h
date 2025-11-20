#ifndef THREAD_POOL_H
#define THREAD_POOL_H

#include <pthread.h>

#define WORKER_QUEUE_SIZE 128

typedef struct {
    int sockets[WORKER_QUEUE_SIZE];
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

void thread_pool_init(thread_pool_t *pool, int num_threads);
void thread_pool_add(thread_pool_t *pool, int client_socket);

#endif
