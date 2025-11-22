#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>

#include "master.h"
#include "config.h"
#include "logger.h"
#include "worker.h"

#define BACKLOG 10

int master_start(void) {

    printf("Master: Configuração:\n");
    printf("- Workers: %d\n", get_num_workers());
    printf("- Threads por worker: %d\n", get_threads_per_worker());
    printf("- Document root: %s\n", get_document_root());
    printf("- Cache: %d MB\n", get_cache_size_mb());

    // --------------------------------------------
    // 1. Criar socket de escuta
    // --------------------------------------------
    int server_socket;
    struct sockaddr_in server_addr;
    int opt = 1;

    server_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (server_socket < 0) {
        perror("socket");
        return -1;
    }

    setsockopt(server_socket, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family      = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port        = htons(get_server_port());

    if (bind(server_socket, (struct sockaddr *)&server_addr,
             sizeof(server_addr)) < 0) {
        perror("bind");
        close(server_socket);
        return -1;
    }

    if (listen(server_socket, BACKLOG) < 0) {
        perror("listen");
        close(server_socket);
        return -1;
    }

    printf("Master: Servidor a ouvir na porta %d\n", get_server_port());

    // --------------------------------------------
    // 2. Criar workers (prefork)
    // --------------------------------------------
    int n_workers = get_num_workers();
    printf("Master: A criar %d workers...\n", n_workers);

    for (int i = 0; i < n_workers; i++) {
        pid_t pid = fork();

        if (pid < 0) {
            perror("fork");
            exit(1);
        }

        if (pid == 0) {
            // Processo filho → worker
            // Cada worker HERDA o server_socket aberto antes do fork
            worker_main(server_socket);
            // nunca volta
            exit(0);
        }
    }

    printf("Master: Workers criados. Master em standby.\n");

    // Opcional: o master pode simplesmente ficar à espera para sempre
    // enquanto os workers tratam das ligações
    while (1) {
        pause();
    }

    // (na prática nunca chega aqui)
    close(server_socket);
    return 0;
}
