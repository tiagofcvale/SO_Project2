#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <errno.h>

#include "http.h"
#include "config.h"

#define MAX_REQ 2048
// Read first order line (ex: "Get /index.html HTTP/1.1")

static int read_request_line(int client_socket, char *method, char *path) {
    char buffer[MAX_REQ];
    int r = recv(client_socket, buffer, sizeof(buffer)-1, 0);

    if (r <= 0) return -1;

    buffer[r] = '\0';

    sscanf(buffer, "%s %s", method, path);

    return 0;
}

// Avoid simple errors (404 / 403)

static void send_error(int client_socket, int code, const char* msg) {
    char body[256];
    sprintf(body, "%d %s\n",code,msg);

    char header[256];

    int h = snprintf(header, sizeof(header),
        "HTTP/1.1 %d %s \r\n"
        "Content-Type: text/plain\r\n"
        "Content-Length: %zu\r\n"
        "Connection: close\r\n"
        "\r\n",
        code, msg, strlen(body)
    );

    send(client_socket, header, h, 0);
    send(client_socket, body, strlen(body), 0);
}

// Principal function: serve file

void http_handle_request(int client_socket) {
    char method[16], path[1024];
    if (read_request_line(client_socket, method, path) < 0) {
        close(client_socket);
        return;
    }

    // accept only GET
    if (strcmp (method, "GET") != 0) {
        send_error(client_socket, 501, "Not Implemented");
        close(client_socket);
        return;
    }

    // Treat "/" as "/index.html"
    if (strcmp(path, "/") == 0) {
        strcpy(path, "/index.html");
    }

    // Build real path
    char fullpath[1024];
    snprintf(fullpath, sizeof(fullpath), "%s%s", get_document_root(), path);

    // Verify file exists
    struct stat st;
    if (stat(fullpath, &st) < 0) {
        send_error(client_socket, 404, "Not Found");
        close(client_socket);
        return;
    }

    // Verify if its directory
    if (S_ISDIR(st.st_mode)) {
        send_error(client_socket, 403, "Forbidden");
        close(client_socket);
        return;
    }

    // Open file
    int fd = open(fullpath, O_RDONLY);
    if (fd < 0) {
        send_error(client_socket, 500, "Internal Error");
        close(client_socket);
        return;
    }

    // Read file
    char filebuf[4096];
    ssize_t n;

    // html header (text/html; charset=utf-8)
    char header[256];
    int h = snprintf(header, sizeof(header),
        "HTTP/1.1 200 OK\r\n"
        "Content-Type: text/html; charset=utf-8\r\n"
        "Content-Length: %ld\r\n"
        "Connection: close\r\n"
        "\r\n",
        st.st_size
    );

    send(client_socket, header, h, 0);

    // Send content
    while ((n = read(fd, filebuf, sizeof(filebuf))) > 0) 
        send(client_socket, filebuf, n, 0);
    
    close(fd);
    close(client_socket);
}