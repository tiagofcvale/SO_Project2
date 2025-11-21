#ifndef HTTP_H
#define HTTP_H

#define MAX_METHOD 16
#define MAX_PATH 1024
#define MAX_VERSION 16
#define MAX_HEADER 512

void http_handle_request(int client_socket);

#endif
