#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>

#include "global.h"
#include "worker.h"
#include "thread_pool.h"
#include "config.h"

// Semáforo simples de accept no processo (mutex do SO)
static pthread_mutex_t accept_mutex = PTHREAD_MUTEX_INITIALIZER;

static thread_pool_t pool;

void worker_main(void) {

    printf("Worker %d iniciado.\n", getpid());

    // Criar thread pool deste worker
    thread_pool_init(&pool, get_threads_per_worker());

    while (1) {

        int client_fd;
        struct sockaddr_in cli;
        socklen_t len = sizeof(cli);

        // -------------------------
        // ACCEPT SINCRONIZADO
        // -------------------------
        pthread_mutex_lock(&accept_mutex);
        client_fd = accept(listen_fd, (struct sockaddr*)&cli, &len);
        pthread_mutex_unlock(&accept_mutex);

        if (client_fd < 0) {
            perror("accept");
            continue;
        }

        printf("Worker %d aceitou socket %d\n", getpid(), client_fd);

        // Enviar socket para a thread pool
        thread_pool_add(&pool, client_fd);
    }
}
