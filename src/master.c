#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <string.h>

#include "master.h"
#include "config.h"
#include "worker.h"
#include "global.h"

#define BACKLOG 128

int master_start(void) {

    struct sockaddr_in addr;
    int opt = 1;

    // Criar listen socket
    listen_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (listen_fd < 0) {
        perror("socket");
        exit(1);
    }

    setsockopt(listen_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    memset(&addr, 0, sizeof(addr));
    addr.sin_family      = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port        = htons(get_server_port());

    if (bind(listen_fd, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        perror("bind");
        exit(1);
    }

    if (listen(listen_fd, BACKLOG) < 0) {
        perror("listen");
        exit(1);
    }

    printf("Master: Servidor a ouvir na porta %d\n", get_server_port());
    printf("Master: A criar %d workers...\n", get_num_workers());

    // Criar workers
    for (int i = 0; i < get_num_workers(); i++) {
        pid_t pid = fork();

        if (pid < 0) {
            perror("fork");
            exit(1);
        }

        if (pid == 0) {
            worker_main();
            exit(0);
        }
    }

    // Master não aceita nada
    printf("Master: Workers criados. Master em standby.\n");

    while (1) pause();
    return 0;
}
