#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>

#include "thread_pool.h"
#include "http.h"

// --------------------------------------------------------------
// queue_pop — usado pelas threads do worker
// --------------------------------------------------------------
static int queue_pop(thread_pool_t *pool) {
    thread_pool_queue_t *q = &pool->queue;

    pthread_mutex_lock(&q->mutex);

    while (q->count == 0)
        pthread_cond_wait(&q->cond_non_empty, &q->mutex);

    int fd = q->sockets[q->front];
    q->front = (q->front + 1) % WORKER_QUEUE_SIZE;
    q->count--;

    pthread_cond_signal(&q->cond_non_full);
    pthread_mutex_unlock(&q->mutex);

    return fd;
}

// --------------------------------------------------------------
// worker_thread — função que cada thread do worker executa
// --------------------------------------------------------------
static void *worker_thread(void *arg) {
    thread_pool_t *pool = arg;

    while (1) {
        int client_socket = queue_pop(pool);

        printf("  [Thread %ld] Recebi socket %d\n",
               pthread_self(), client_socket);

        http_handle_request(client_socket);
    }

    return NULL;
}

// --------------------------------------------------------------
// thread_pool_add — coloca um socket na queue local do worker
// --------------------------------------------------------------
void thread_pool_add(thread_pool_t *pool, int client_socket) {
    thread_pool_queue_t *q = &pool->queue;

    pthread_mutex_lock(&q->mutex);

    while (q->count == WORKER_QUEUE_SIZE)
        pthread_cond_wait(&q->cond_non_full, &q->mutex);

    q->sockets[q->rear] = client_socket;
    q->rear = (q->rear + 1) % WORKER_QUEUE_SIZE;
    q->count++;

    pthread_cond_signal(&q->cond_non_empty);
    pthread_mutex_unlock(&q->mutex);
}

// --------------------------------------------------------------
// thread_pool_init — cria a queue interna e as threads do worker
// --------------------------------------------------------------
void thread_pool_init(thread_pool_t *pool, int n) {

    // Inicializar queue interna do worker
    pool->queue.front = 0;
    pool->queue.rear  = 0;
    pool->queue.count = 0;

    pthread_mutex_init(&pool->queue.mutex, NULL);
    pthread_cond_init(&pool->queue.cond_non_empty, NULL);
    pthread_cond_init(&pool->queue.cond_non_full, NULL);

    // Criar threads
    pool->thread_count = n;
    pool->threads = malloc(sizeof(pthread_t) * n);

    for (int i = 0; i < n; i++) {
        pthread_create(&pool->threads[i], NULL, worker_thread, pool);
    }

    printf("Worker process criou %d threads.\n", n);
}
