#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include "master.h"
#include "config.h"
#include "logger.h"

#define BACKLOG 10
#define BUFFER_SIZE 4096

static int server_socket = -1;

int master_start(void) {
    struct sockaddr_in server_addr;
    int opt = 1;
    
    // Criar socket
    server_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (server_socket < 0) {
        perror("socket");
        return -1;
    }
    
    // Configurar socket para reutilizar endereço
    if (setsockopt(server_socket, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt))) {
        perror("setsockopt");
        close(server_socket);
        return -1;
    }
    
    // Configurar endereço do servidor
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(get_server_port());
    
    // Bind
    if (bind(server_socket, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        perror("bind");
        close(server_socket);
        return -1;
    }
    
    // Listen
    if (listen(server_socket, BACKLOG) < 0) {
        perror("listen");
        close(server_socket);
        return -1;
    }
    
    printf("Servidor iniciado na porta %d\n", get_server_port());
    printf("Aguardando conexões...\n");
    
    // Loop principal - aceitar conexões
    while (1) {
        struct sockaddr_in client_addr;
        socklen_t client_len = sizeof(client_addr);
        int client_socket;
        
        // Aceitar conexão
        client_socket = accept(server_socket, (struct sockaddr*)&client_addr, &client_len);
        if (client_socket < 0) {
            perror("accept");
            continue;
        }
        
        printf("Cliente conectado: %s:%d\n", 
               inet_ntoa(client_addr.sin_addr), 
               ntohs(client_addr.sin_port));
        
        // Enviar resposta HTTP simples
        char response[] = "HTTP/1.1 200 OK\r\n"
                         "Content-Type: text/plain\r\n"
                         "Content-Length: 12\r\n"
                         "\r\n"
                         "Hello World!";
        
        send(client_socket, response, strlen(response), 0);
        close(client_socket);
        
        printf("Resposta enviada, conexão fechada\n");
    }
    
    return 0;
}

int master_stop(void) {
    if (server_socket != -1) {
        close(server_socket);
        server_socket = -1;
    }
    return 0;
}