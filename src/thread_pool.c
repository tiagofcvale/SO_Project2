#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>

#include "thread_pool.h"
#include "http.h"

/**
 * @brief Removes and returns a connection from the work queue (consumer).
 * @param pool Pointer to the thread pool.
 * @return Pointer to the connection removed from the queue.
 */
static connection_t* queue_pop(thread_pool_t *pool) {
    thread_pool_queue_t *q = &pool->queue;

    pthread_mutex_lock(&q->mutex);

    while (q->count == 0)
        pthread_cond_wait(&q->cond_non_empty, &q->mutex);

    connection_t* conn = q->connections[q->front];
    q->front = (q->front + 1) % WORKER_QUEUE_SIZE;
    q->count--;

    pthread_cond_signal(&q->cond_non_full);
    pthread_mutex_unlock(&q->mutex);

    return conn;
}

/**
 * @brief Function executed by each thread in the pool.
 * @param arg Pointer to the thread pool (thread_pool_t*).
 * @return NULL (never normally returns).
 */
static void *worker_thread(void *arg) {
    thread_pool_t *pool = arg;

    while (1) {
        connection_t* conn = queue_pop(pool);

         printf("  [Thread %ld] Received connection fd=%d (HTTPS=%d)\n",
             pthread_self(), conn->fd, conn->is_https);

        http_handle_request(conn);
    }

    return NULL;
}

/**
 * @brief Adds a connection to the work queue (producer).
 * @param pool Pointer to the thread pool.
 * @param conn Pointer to the connection to add to the queue.
 */
void thread_pool_add(thread_pool_t *pool, connection_t* conn) {
    thread_pool_queue_t *q = &pool->queue;

    pthread_mutex_lock(&q->mutex);

    while (q->count == WORKER_QUEUE_SIZE)
        pthread_cond_wait(&q->cond_non_full, &q->mutex);

    q->connections[q->rear] = conn;
    q->rear = (q->rear + 1) % WORKER_QUEUE_SIZE;
    q->count++;

    pthread_cond_signal(&q->cond_non_empty);
    pthread_mutex_unlock(&q->mutex);
}

/**
 * @brief Initializes the thread pool and the internal queue.
 * @param pool Pointer to the thread pool to initialize.
 * @param n Number of threads to create in the pool.
 */
void thread_pool_init(thread_pool_t *pool, int n) {

    // Initialize internal queue
    pool->queue.front = 0;
    pool->queue.rear  = 0;
    pool->queue.count = 0;

    pthread_mutex_init(&pool->queue.mutex, NULL);
    pthread_cond_init(&pool->queue.cond_non_empty, NULL);
    pthread_cond_init(&pool->queue.cond_non_full, NULL);

    // Create threads
    pool->thread_count = n;
    pool->threads = malloc(sizeof(pthread_t) * n);

    for (int i = 0; i < n; i++) {
        pthread_create(&pool->threads[i], NULL, worker_thread, pool);
    }

    printf("Worker process created %d threads.\n", n);
}