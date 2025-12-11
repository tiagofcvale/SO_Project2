#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <unistd.h>
#include <sys/socket.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <errno.h>
#include <sys/time.h>
#include <openssl/ssl.h>
#include <openssl/err.h>
#include <sys/sendfile.h>

#include "http.h"
#include "config.h"
#include "logger.h"
#include "cache.h"
#include "stats.h"
#include "shared_mem.h"
#include "semaphores.h"

#define MAX_REQ 2048
#define MAX_REQ_LINE 2048

// External references to shared memory and semaphores from worker.c
extern shared_data_t* shm_data;
extern ipc_semaphores_t sems;

/**
 * @brief Returns the appropriate MIME type for a file based on its extension.
 * @param path File path.
 * @return String with the corresponding MIME type.
 */
static const char* mime_from_path(const char* path) {

    const char* ext = strrchr(path, '.');
    if (!ext) return "application/octet-stream";
    ext++;

    if (!strcasecmp(ext, "html")) return "text/html; charset=utf-8";
    if (!strcasecmp(ext, "htm"))  return "text/html; charset=utf-8";
    if (!strcasecmp(ext, "css"))  return "text/css";
    if (!strcasecmp(ext, "js"))   return "application/javascript";
    if (!strcasecmp(ext, "json")) return "application/json; charset=utf-8";
    if (!strcasecmp(ext, "png"))  return "image/png";
    if (!strcasecmp(ext, "jpg"))  return "image/jpeg";
    if (!strcasecmp(ext, "jpeg")) return "image/jpeg";
    if (!strcasecmp(ext, "gif"))  return "image/gif";
    if (!strcasecmp(ext, "svg"))  return "image/svg+xml";
    if (!strcasecmp(ext, "txt"))  return "text/plain; charset=utf-8";
    if (!strcasecmp(ext, "pdf"))  return "application/pdf";

    return "application/octet-stream";
}

/**
 * @brief Lê dados de uma conexão (HTTP ou HTTPS)
 */
static ssize_t conn_read(connection_t* conn, void* buf, size_t len) {
    if (conn->is_https && conn->ssl) {
        return SSL_read(conn->ssl, buf, len);
    } else {
        return recv(conn->fd, buf, len, 0);
    }
}

/**
 * @brief Escreve dados numa conexão (HTTP ou HTTPS)
 */
static ssize_t conn_write(connection_t* conn, const void* buf, size_t len) {
    if (conn->is_https && conn->ssl) {
        return SSL_write(conn->ssl, buf, len);
    } else {
        return send(conn->fd, buf, len, 0);
    }
}

/**
 * @brief Fecha uma conexão e liberta recursos
 */
static void conn_close(connection_t* conn) {
    if (conn->is_https && conn->ssl) {
        SSL_shutdown(conn->ssl);
        SSL_free(conn->ssl);
    }
    close(conn->fd);
    free(conn);
}



/**
 * @brief Reads a line from the socket non-blocking, until '\n' or reaching the maximum.
 * @param fd Socket descriptor.
 * @param buf Destination buffer for the read line.
 * @param max Maximum buffer size.
 * @return Number of bytes read, or -1 on error.
 */
static int read_line_conn(connection_t* conn, char* buf, int max) {
    int i = 0;
    char c;

    while (i < max - 1) {
        int n = conn_read(conn, &c, 1);

        if (n == 0) {
            // cliente fechou a ligação
            break;
        }

        if (n < 0) {
            // interrupções do sistema → tentar de novo
            if (errno == EINTR) continue;

            // socket não tem dados ainda → tentar de novo
            if (errno == EAGAIN || errno == EWOULDBLOCK) continue;

            return -1;
        }

        buf[i++] = c;

        if (c == '\n')
            break;
    }

    buf[i] = '\0';
    return i;
}


/**
 * @brief Parses the HTTP request, separating method, path, and headers.
 * @param client_fd Client socket descriptor.
 * @param req Structure where the request data will be stored.
 * @return 0 on success, -1 on error.
 */
static int parse_request_conn(connection_t* conn, http_request_t* req) {
    char line[MAX_REQ_LINE];

    printf("[PARSE] A ler request...\n");

    // Primeira linha
    int n = read_line_conn(conn, line, sizeof(line));
    printf("[PARSE] First line raw: '%s' (n=%d)\n", line, n);

    if (n <= 0) return -1;

    sscanf(line, "%7s %1023s %15s", req->method, req->path, req->version);
    printf("[PARSE] Método='%s' Path='%s' Versão='%s'\n",
           req->method, req->path, req->version);

    // Headers
    while (1) {
        n = read_line_conn(conn, line, sizeof(line));
        printf("[PARSE] Header line: '%s' (n=%d)\n", line, n);

        if (n <= 0) return -1;

        if (!strcmp(line, "\r\n")) {
            printf("[PARSE] Fim dos headers\n");
            break;
        }

        sscanf(line, "Host: %511[^\r\n]", req->host);
        sscanf(line, "User-Agent: %511[^\r\n]", req->user_agent);
        sscanf(line, "Accept: %511[^\r\n]", req->accept);
    }

    return 0;
}




/**
 * @brief Sends a custom or generic HTTP error page to the client.
 * @param fd Client socket descriptor.
 * @param code HTTP error code (e.g., 404, 500).
 * @param msg Message associated with the error.
 */
static void send_error_page_conn(connection_t* conn, int code, const char* msg) {
    char errpath[256];
    snprintf(errpath, sizeof(errpath), "%s/errors/%d.html",
             get_document_root(), code);

    int f = open(errpath, O_RDONLY);

    if (f >= 0) {
        struct stat st;
        fstat(f, &st);

        char header[256];
        int h = snprintf(header, sizeof(header),
            "HTTP/1.1 %d %s\r\n"
            "Content-Type: text/html; charset=utf-8\r\n"
            "Content-Length: %ld\r\n"
            "Connection: close\r\n"
            "\r\n",
            code, msg, st.st_size
        );

        conn_write(conn, header, h);

        char buf[4096];
        ssize_t n;
        while ((n = read(f, buf, sizeof(buf))) > 0)
            conn_write(conn, buf, n);

        close(f);
        
        if (shm_data) {
            stats_update(&shm_data->stats, sems.sem_stats, code, st.st_size);
        }
        
        return;
    }

    // fallback
    char body[256];
    int blen = snprintf(body, sizeof(body),
        "<html><body><h1>%d %s</h1></body></html>", code, msg);

    char header[256];
    int h = snprintf(header, sizeof(header),
        "HTTP/1.1 %d %s\r\n"
        "Content-Type: text/html; charset=utf-8\r\n"
        "Content-Length: %d\r\n"
        "Connection: close\r\n"
        "\r\n",
        code, msg, blen
    );

    conn_write(conn, header, h);
    conn_write(conn, body, blen);
    
    if (shm_data) {
        stats_update(&shm_data->stats, sems.sem_stats, code, blen);
    }
}


/**
 * @brief Serves a file to the client, using the cache if possible.
 * @param fd Client socket descriptor.
 * @param fullpath Absolute path of the file to serve.
 * @param is_head If 1, sends only headers (HEAD method), if 0 sends body as well (GET).
 */
static void serve_file_conn(connection_t* conn, const char *fullpath, int is_head) {

    printf("[SERVE] fullpath='%s', is_head=%d\n", fullpath, is_head);

    char* cached_data = NULL;
    size_t cached_size = 0;

    if (cache_get(fullpath, &cached_data, &cached_size)) {

    const char* mime = mime_from_path(fullpath);

    char header[256];
    int h = snprintf(header, sizeof(header),
        "HTTP/1.1 200 OK\r\n"
        "Content-Type: %s\r\n"
        "Content-Length: %zu\r\n"
        "Connection: keep-alive\r\n\r\n",
        mime, cached_size
    );

    // Enviar cabeçalho
    conn_write(conn, header, h);

    // Se for HEAD, não enviar corpo
    if (!is_head) {
        conn_write(conn, cached_data, cached_size);
    }

    return;
}

    // Cache miss - no specific tracking needed, just counted in final stats

    int file_fd = open(fullpath, O_RDONLY);
    printf("[SERVE] open() file_fd=%d\n", file_fd);

    if (file_fd < 0) {
    send_error_page_conn(conn, 500, "Internal Server Error");
    return;
}

struct stat st;
if (stat(fullpath, &st) < 0) {
    close(file_fd);
    send_error_page_conn(conn, 500, "Internal Server Error");
    return;
}

    const char* mime = mime_from_path(fullpath);

    char header[256];
    int h = snprintf(header, sizeof(header),
        "HTTP/1.1 200 OK\r\n"
        "Content-Type: %s\r\n"
        "Content-Length: %ld\r\n"
        "Connection: close\r\n"
        "\r\n",
        mime, st.st_size
    );
    printf("[SERVE] header sent (%d bytes)\n", h);


    conn_write(conn, header, h);

    if (!is_head) {
        ssize_t n = sendfile(conn->fd, file_fd, NULL, st.st_size);
        if (n < 0) {
            send_error_page_conn(conn, 500, "Internal Server Error");
        }
    }

    close(file_fd);

    printf("[SERVE] file sent (%ld bytes)\n", st.st_size);


    char* file_data = malloc(st.st_size);
    if (!file_data) {

        char buf[4096];
        ssize_t n;
        size_t total_sent = 0;
        while ((n = read(file_fd, buf, sizeof(buf))) > 0) {
            conn_write(conn, buf, n);
        }


        close(file_fd);
        
        // Update stats
        if (shm_data) {
            stats_update(&shm_data->stats, sems.sem_stats, 200, total_sent);
        }
        
        return;
    }

    ssize_t total = read(file_fd, file_data, st.st_size);
    close(file_fd);

    if (total != st.st_size) {
        free(file_data);
        send_error_page_conn(conn, 500, "Internal Server Error");

        return;
    }

    conn_write(conn, file_data, st.st_size);


    cache_put(fullpath, file_data, st.st_size);

    // Update stats - successful file served (status 200)
    if (shm_data) {
        stats_update(&shm_data->stats, sems.sem_stats, 200, st.st_size);
    }

    free(file_data);
}


/**
 * @brief Main function to handle an HTTP request from a client.
 *        Parses, validates, serves files, and logs statistics.
 * @param client_socket Client socket descriptor.
 */
void http_handle_request(connection_t* conn) {
    printf("[HTTP] Entrou no http_handle_request com fd %d (HTTPS=%d)\n", 
           conn->fd, conn->is_https);

    http_request_t req = {0};

    struct sockaddr_in addr;
    socklen_t addrlen = sizeof(addr);
    getpeername(conn->fd, (struct sockaddr *)&addr, &addrlen);
    snprintf(req.client_ip, sizeof(req.client_ip),
             "%s", inet_ntoa(addr.sin_addr));

    if (parse_request_conn(conn, &req) < 0) {
        send_error_page_conn(conn, 400, "Bad Request");
        logger_log(req.client_ip, "-", "-", 400, 0);
        conn_close(conn);
        return;
    }

    // Validar método
    int is_head = 0;
    if (strcmp(req.method, "GET") == 0) {
        is_head = 0;
    } else if (strcmp(req.method, "HEAD") == 0) {
        is_head = 1;
    } else {
        send_error_page_conn(conn, 501, "Not Implemented");
        logger_log(req.client_ip, req.method, req.path, 501, 0);
        conn_close(conn);
        return;
    }

    if (!strcmp(req.path, "/"))
        strcpy(req.path, "/index.html");

    char fullpath[1024];
    snprintf(fullpath, sizeof(fullpath), "%s%s",
             get_document_root(), req.path);

    struct stat st;
    if (stat(fullpath, &st) < 0) {
        send_error_page_conn(conn, 404, "Not Found");
        logger_log(req.client_ip, req.method, req.path, 404, 0);
        conn_close(conn);
        return;
    }

    if (S_ISDIR(st.st_mode)) {
        send_error_page_conn(conn, 403, "Forbidden");
        logger_log(req.client_ip, req.method, req.path, 403, 0);
        conn_close(conn);
        return;
    }

    serve_file_conn(conn, fullpath, is_head);
    logger_log(req.client_ip, req.method, req.path, 200, st.st_size);

    conn_close(conn);
}