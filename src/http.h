#ifndef HTTP_H
#define HTTP_H

#include <stddef.h>
#include "worker.h"  // Para connection_t

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
// Agora recebe connection_t em vez de int
void http_handle_request(connection_t* conn);

#endif