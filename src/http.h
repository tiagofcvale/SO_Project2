#ifndef HTTP_H
#define HTTP_H

#include <stddef.h>

// Structure with the relevant fields of the HTTP request
typedef struct {
    char method[8];
    char path[1024];
    char version[16];

    char host[512];
    char user_agent[512];
    char accept[512];

    char client_ip[64];
} http_request_t;


// Main function called by each worker thread
void http_handle_request(int client_socket);

#endif
