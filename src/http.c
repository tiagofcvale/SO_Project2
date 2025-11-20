#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <errno.h>

#include "http.h"

// ---------------------------------------------------------------------
// http_handle_request — envia sempre uma resposta HTTP válida
// ---------------------------------------------------------------------
void http_handle_request(int client_socket) {

    char buffer[1024];

    // Ler o pedido sem bloquear (Chrome envia headers em vários pacotes)
    int r = recv(client_socket, buffer, sizeof(buffer) - 1, MSG_DONTWAIT);
    if (r > 0)
        buffer[r] = '\0'; // garantir fim de string

    const char *body = "Hello from Worker Thread!\n";
    size_t body_len = strlen(body);

    // Construir os headers
    char header[256];
    int header_len = snprintf(
        header,
        sizeof(header),
        "HTTP/1.1 200 OK\r\n"
        "Content-Type: text/plain\r\n"
        "Content-Length: %zu\r\n"
        "Connection: close\r\n"
        "\r\n",
        body_len
    );

    // Enviar header
    int s1 = send(client_socket, header, header_len, 0);
    if (s1 < 0) {
        perror("[HTTP] send header");
        close(client_socket);
        return;
    }

    // Enviar body
    int s2 = send(client_socket, body, body_len, 0);
    if (s2 < 0) {
        perror("[HTTP] send body");
        close(client_socket);
        return;
    }

    // Forçar flush do kernel
    shutdown(client_socket, SHUT_WR);

    // Fechar ligação
    close(client_socket);
}
