#ifndef HTTP_H
#define HTTP_H

#define MAX_METHOD 16
#define MAX_PATH 1024
#define MAX_VERSION 16
#define MAX_HEADER 512

// HTTP order structure
typedef struct {
    char method[MAX_METHOD];
    char path[MAX_PATH];
    char version[MAX_VERSION];

    // util headers
    char host[MAX_HEADER];
    char user_agent[MAX_HEADER];
    char accept[MAX_HEADER];

    // Client IP
    char client_ip[64];

} http_request_t;

void http_handle_request(int client_socket);

#endif
