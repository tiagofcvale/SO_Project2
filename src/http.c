#include <stdio.h>
#include <string.h>
#include <strings.h>
#include <unistd.h>
#include <sys/socket.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <errno.h>

#include "http.h"
#include "config.h"

#define MAX_REQ 2048
#define MAX_REQ_LINE 2048

// MIME file

static const char* mime_from_path(const char* path) {

    // Find last extention
    const char* ext = strrchr(path, '.');
    if (!ext) return "application/octet_stream";

    ext++;  // skip '.'

    if (strcasecmp(ext, "html") == 0) return "text/html; charset=utf-8";
    if (strcasecmp(ext, "htm")  == 0) return "text/html; charset=utf-8";
    if (strcasecmp(ext, "css")  == 0) return "text/css";
    if (strcasecmp(ext, "js")   == 0) return "application/javascript";
    if (strcasecmp(ext, "json") == 0) return "application/json; charset=utf-8";
    if (strcasecmp(ext, "png")  == 0) return "image/png";
    if (strcasecmp(ext, "jpg")  == 0) return "image/jpeg";
    if (strcasecmp(ext, "jpeg") == 0) return "image/jpeg";
    if (strcasecmp(ext, "gif")  == 0) return "image/gif";
    if (strcasecmp(ext, "svg")  == 0) return "image/svg+xml";
    if (strcasecmp(ext, "txt")  == 0) return "text/plain; charset=utf-8";
    if (strcasecmp(ext, "pdf")  == 0) return "application/pdf";

    return "application/octet-stream";  // default
}

// Read line until '\n'

static int read_line(int fd, char* buf, int max) {
    int i = 0;

    while (i < max) {
        char c;
        int n = recv(fd, &c, 1, 0);
        if (n <= 0) return -1;
        
        buf[i++] = c;

        if (c == '\n') break;
    }

    buf[i] = '\0';
    return i;
}

// Parser of http request

static int parse_request(int client_fd, http_request_t* req) {
    char line[MAX_REQ_LINE];

    // First Line: GET /x HTTP/1.1
    if (read_line(client_fd, line, sizeof(line)) <= 0) 
        return -1;
    
    sscanf(line, "%s %s %s", req->method, req->path, req->version);

    // Headers per line
    while (1) {
        if (read_line(client_fd, line, sizeof(line)) <= 0)
            return -1;
        
        // empty line: end of headers
        if (strcmp(line, "\r\n") == 0)
            break;
        
        if (sscanf(line, "Host: %511[^\r\n]", req->host) == 1)
            continue;

        if (sscanf(line, "User-Agent: %511[^\r\n]", req->user_agent) == 1)
            continue;

        if (sscanf(line, "Accept: %511[^\r\n]", req->accept) == 1)
            continue;
    }

    return 0;
}

// HTTP errors with personalized html

static void send_error_page(int fd, int code, const char* msg) {
    char path[256];
    snprintf(path, sizeof(path), "%s/errors/%d.html",
             get_document_root(), code);

    int f = open(path, O_RDONLY);

    // If exists, serve
    if (f>=0) {
        struct stat st;
        fstat(f, &st);

        char header[256];
        int h = snprintf(header, sizeof(header),
            "HTTP/1.1 %d %s\r\n"
            "Content-Type: text/html; charset=uth-8\r\n"
            "Content-Length: %ld\r\n"
            "Connection: close\r\n"
            "\r\n",
            code, msg, st.st_size
        );

        send(fd, header, h, 0);

        char buf[4096];
        ssize_t n;

        while ((n = read(f, buf, sizeof(buf))) > 0)
            send(fd, buf, n, 0);
        
        close(f);
        return;
    }
}

// Serve file

static void serve_file(int fd, const char *fullpath) {
    int file_fd = open(fullpath, O_RDONLY);
    if (file_fd < 0) {
        send_error_page(fd, 500, "Internal Server Error");
        return;
    }

    struct stat st;
    if (stat(fullpath, &st) < 0) {
        close(file_fd);
        send_error_page(fd, 500, "Internal Server Error");
        return;
    }

    const char *mime = mime_from_path(fullpath);

    char header[256];
    int h = snprintf(header, sizeof(header),
        "HTTP/1.1 200 OK\r\n"
        "Content-Type: %s\r\n"
        "Content-Length: %ld\r\n"
        "Connection: close\r\n"
        "\r\n",
        mime,
        st.st_size
    );

    send(fd, header, h, 0);

    // Send file
    char buf[4096];
    ssize_t n;

    while ((n = read(file_fd, buf, sizeof(buf))) > 0) {
        send(fd, buf, n, 0);
    }

    close(file_fd);
}

// Principal function: serve file

void http_handle_request(int client_socket) {
    http_request_t req = {0};

    if (parse_request(client_socket, &req) < 0) {
        close(client_socket);
        return;
    }

    // Only GET for now
    if (strcmp(req.method, "GET") != 0) {
        send_error_page(client_socket, 501, "Not Implemented");
        close(client_socket);
        return;
    }

    // index.html
    if (strcmp(req.path, "/") == 0)
        strcpy(req.path, "/index.html");

    // Build real path
    char fullpath[1024];
    snprintf(fullpath, sizeof(fullpath), "%s%s",
             get_document_root(), req.path);

    // Verify existence
    struct stat st;
    if (stat(fullpath, &st) < 0) {
        send_error_page(client_socket, 404, "Not Found");
        close(client_socket);
        return;
    }

    // If its directory 403
    if (S_ISDIR(st.st_mode)) {
        send_error_page(client_socket, 403, "Forbidden");
        close(client_socket);
        return;
    }

    // Serve file
    serve_file(client_socket, fullpath);

    close(client_socket);
}