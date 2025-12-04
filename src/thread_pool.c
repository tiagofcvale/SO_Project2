#define _POSIX_C_SOURCE 200809L

#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/socket.h>

#include "thread_pool.h"
#include "http.h"

/**
 * @brief Removes and returns a socket from the work queue (consumer).
 * @param pool Pointer to the thread pool.
 * @return Socket descriptor taken from the queue.
 */
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

/**
 * @brief Function executed by each thread in the pool.
 * @param arg Pointer to the thread pool (thread_pool_t*).
 * @return NULL (never normally returns).
 */
static void *worker_thread(void *arg) {
    thread_pool_t *pool = arg;

    while (1) {
        int client_socket = queue_pop(pool);

        printf("  [Thread %ld] Received socket %d\n", 

               pthread_self(), client_socket);

        http_handle_request(client_socket);   
    }

    return NULL;
}

/**
 * @brief Adds a socket to the work queue with timeout (producer).
 * @param pool Pointer to the thread pool.
 * @param client_socket Socket descriptor to add to the queue.
 * @return 0 on success, -1 if timeout/failure.
 */
int thread_pool_add(thread_pool_t *pool, int client_socket) {
    thread_pool_queue_t *q = &pool->queue;

    pthread_mutex_lock(&q->mutex);

    // Wait up to 2 seconds for space in the queue
    struct timespec ts;
    clock_gettime(CLOCK_REALTIME, &ts);
    ts.tv_sec += 2;  // 2 second timeout

    while (q->count == WORKER_QUEUE_SIZE) {
        int ret = pthread_cond_timedwait(&q->cond_non_full, &q->mutex, &ts);
        
        if (ret == ETIMEDOUT) {
            pthread_mutex_unlock(&q->mutex);
            fprintf(stderr, "[WARN] Thread pool queue full - rejecting connection\n");
            
            // Send 503 and close socket
            const char *resp = "HTTP/1.1 503 Service Unavailable\r\n"
                             "Content-Type: text/plain\r\n"
                             "Connection: close\r\n\r\n"
                             "Server too busy\n";
            send(client_socket, resp, strlen(resp), 0);
            close(client_socket);
            return -1;
        }
    }

    q->sockets[q->rear] = client_socket;
    q->rear = (q->rear + 1) % WORKER_QUEUE_SIZE;
    q->count++;

    pthread_cond_signal(&q->cond_non_empty);
    pthread_mutex_unlock(&q->mutex);
    
    return 0;
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