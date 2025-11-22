#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include "config.h"
#include "worker.h"
#include "thread_pool.h"

void worker_main(int listen_fd) {
    // criar thread pool local deste worker
    thread_pool_t pool;
    thread_pool_init(&pool, get_threads_per_worker());

    printf("Worker %d iniciado.\n", getpid());

    while (1) {
        struct sockaddr_in client_addr;
        socklen_t len = sizeof(client_addr);

        // cada worker faz accept() directamente
        int client_socket = accept(listen_fd,
                                   (struct sockaddr *)&client_addr,
                                   &len);

        if (client_socket < 0) {
            perror("worker accept");
            continue;
        }

        printf("Worker %d recebeu socket %d de %s:%d\n",
               getpid(),
               client_socket,
               inet_ntoa(client_addr.sin_addr),
               ntohs(client_addr.sin_port));

        // mandar o socket para uma thread deste worker
        thread_pool_add(&pool, client_socket);
        // a thread é que faz http_handle_request() e close()
    }
}
