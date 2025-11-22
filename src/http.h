#ifndef HTTP_H
#define HTTP_H

#include <stddef.h>

// Estrutura com os campos relevantes do pedido HTTP
typedef struct {
    char method[8];
    char path[1024];
    char version[16];

    char host[512];
    char user_agent[512];
    char accept[512];

    char client_ip[64];
} http_request_t;


// Função principal chamada pelas threads de cada worker
void http_handle_request(int client_socket);

#endif
