#ifndef SSL_WRAPPER_H
#define SSL_WRAPPER_H

#include <openssl/ssl.h>
#include <openssl/err.h>

/*
 * Estrutura que guarda o contexto SSL do servidor.
 */
typedef struct {
    SSL_CTX *ctx;
} ssl_server_ctx_t;

/**
 * Inicializa a biblioteca OpenSSL e cria um contexto SSL.
 */
ssl_server_ctx_t* ssl_server_init(const char *cert_path, const char *key_path);

/**
 * Liberta o contexto SSL.
 */
void ssl_server_cleanup(ssl_server_ctx_t *server_ctx);

/**
 * Cria um objeto SSL associado ao file descriptor da ligação.
 */
SSL* ssl_create_for_fd(ssl_server_ctx_t *server_ctx, int client_fd);

int ssl_perform_handshake(SSL *ssl);

int ssl_read_wrapper(SSL *ssl, void *buf, int size);

int ssl_write_wrapper(SSL *ssl, const void *buf, int size);

void ssl_close_connection(SSL *ssl);

#endif
