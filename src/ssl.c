#include "ssl.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <openssl/ssl.h>
#include <openssl/err.h>

ssl_server_ctx_t* ssl_server_init(const char *cert_path, const char *key_path)
{
    printf("[SSL] A inicializar OpenSSL...\n");
    
    // Inicializar biblioteca OpenSSL
    SSL_load_error_strings();
    OpenSSL_add_ssl_algorithms();
    ERR_load_crypto_strings();

    ssl_server_ctx_t *server_ctx = malloc(sizeof(ssl_server_ctx_t));
    if (!server_ctx) {
        perror("malloc ssl_server_ctx");
        return NULL;
    }

    // Criar contexto SSL com método TLS genérico
    const SSL_METHOD *method = TLS_server_method();
    server_ctx->ctx = SSL_CTX_new(method);
    
    if (!server_ctx->ctx) {
        fprintf(stderr, "[SSL] Erro ao criar SSL_CTX\n");
        ERR_print_errors_fp(stderr);
        free(server_ctx);
        return NULL;
    }

    // Configurar opções SSL
    SSL_CTX_set_options(server_ctx->ctx, 
                        SSL_OP_NO_SSLv2 | SSL_OP_NO_SSLv3 | 
                        SSL_OP_NO_COMPRESSION);
    
    // Definir versão mínima do TLS
    SSL_CTX_set_min_proto_version(server_ctx->ctx, TLS1_2_VERSION);

    printf("[SSL] A carregar certificado: %s\n", cert_path);
    
    // Carregar certificado
    if (SSL_CTX_use_certificate_file(server_ctx->ctx, cert_path, SSL_FILETYPE_PEM) <= 0) {
        fprintf(stderr, "[SSL] ERRO: Falha ao carregar certificado '%s'\n", cert_path);
        ERR_print_errors_fp(stderr);
        SSL_CTX_free(server_ctx->ctx);
        free(server_ctx);
        return NULL;
    }
    
    printf("[SSL] Certificado carregado com sucesso\n");
    printf("[SSL] A carregar chave privada: %s\n", key_path);

    // Carregar chave privada
    if (SSL_CTX_use_PrivateKey_file(server_ctx->ctx, key_path, SSL_FILETYPE_PEM) <= 0) {
        fprintf(stderr, "[SSL] ERRO: Falha ao carregar chave privada '%s'\n", key_path);
        ERR_print_errors_fp(stderr);
        SSL_CTX_free(server_ctx->ctx);
        free(server_ctx);
        return NULL;
    }
    
    printf("[SSL] Chave privada carregada com sucesso\n");

    // Verificar se a chave e o certificado correspondem
    if (!SSL_CTX_check_private_key(server_ctx->ctx)) {
        fprintf(stderr, "[SSL] ERRO CRÍTICO: Certificado e chave privada não correspondem!\n");
        ERR_print_errors_fp(stderr);
        SSL_CTX_free(server_ctx->ctx);
        free(server_ctx);
        return NULL;
    }

    printf("[SSL] ✓ Certificado e chave verificados e correspondentes\n");
    printf("[SSL] ✓ SSL inicializado com sucesso\n");

    return server_ctx;
}

void ssl_server_cleanup(ssl_server_ctx_t *server_ctx)
{
    if (!server_ctx) return;

    printf("[SSL] A limpar recursos SSL...\n");
    
    SSL_CTX_free(server_ctx->ctx);
    EVP_cleanup();
    ERR_free_strings();
    
    free(server_ctx);
    
    printf("[SSL] Recursos SSL libertados\n");
}

SSL* ssl_create_for_fd(ssl_server_ctx_t *server_ctx, int client_fd)
{
    if (!server_ctx || !server_ctx->ctx) {
        fprintf(stderr, "[SSL] ssl_create_for_fd: contexto inválido\n");
        return NULL;
    }

    SSL *ssl = SSL_new(server_ctx->ctx);
    if (!ssl) {
        fprintf(stderr, "[SSL] Erro ao criar objeto SSL\n");
        ERR_print_errors_fp(stderr);
        return NULL;
    }

    if (SSL_set_fd(ssl, client_fd) == 0) {
        fprintf(stderr, "[SSL] Erro ao associar SSL ao FD %d\n", client_fd);
        ERR_print_errors_fp(stderr);
        SSL_free(ssl);
        return NULL;
    }

    return ssl;
}

int ssl_perform_handshake(SSL *ssl)
{
    if (!ssl) {
        fprintf(stderr, "[SSL] ssl_perform_handshake: SSL é NULL\n");
        return 0;
    }

    int ret = SSL_accept(ssl);
    
    if (ret <= 0) {
        int err = SSL_get_error(ssl, ret);
        
        fprintf(stderr, "[SSL] Handshake falhou (ret=%d, err=%d)\n", ret, err);
        
        switch (err) {
            case SSL_ERROR_WANT_READ:
                fprintf(stderr, "[SSL] SSL_ERROR_WANT_READ\n");
                break;
            case SSL_ERROR_WANT_WRITE:
                fprintf(stderr, "[SSL] SSL_ERROR_WANT_WRITE\n");
                break;
            case SSL_ERROR_SYSCALL:
                fprintf(stderr, "[SSL] SSL_ERROR_SYSCALL\n");
                perror("SSL syscall");
                break;
            case SSL_ERROR_SSL:
                fprintf(stderr, "[SSL] SSL_ERROR_SSL (protocol error)\n");
                break;
            default:
                fprintf(stderr, "[SSL] Erro SSL desconhecido: %d\n", err);
        }
        
        ERR_print_errors_fp(stderr);
        return 0;
    }
    
    printf("[SSL] Handshake bem sucedido (cipher: %s)\n", SSL_get_cipher(ssl));
    return 1;
}

int ssl_read_wrapper(SSL *ssl, void *buf, int size)
{
    if (!ssl) return -1;
    
    int n = SSL_read(ssl, buf, size);

    if (n <= 0) {
        int err = SSL_get_error(ssl, n);
        if (err == SSL_ERROR_ZERO_RETURN) {
            // Conexão fechada normalmente
            return 0;
        }
        if (err != SSL_ERROR_WANT_READ && err != SSL_ERROR_WANT_WRITE) {
            fprintf(stderr, "[SSL] Erro no SSL_read (err=%d)\n", err);
            ERR_print_errors_fp(stderr);
        }
        return -1;
    }

    return n;
}

int ssl_write_wrapper(SSL *ssl, const void *buf, int size)
{
    if (!ssl) return -1;
    
    int n = SSL_write(ssl, buf, size);

    if (n <= 0) {
        int err = SSL_get_error(ssl, n);
        if (err == SSL_ERROR_ZERO_RETURN) {
            return 0;
        }
        if (err != SSL_ERROR_WANT_READ && err != SSL_ERROR_WANT_WRITE) {
            fprintf(stderr, "[SSL] Erro no SSL_write (err=%d)\n", err);
            ERR_print_errors_fp(stderr);
        }
        return -1;
    }

    return n;
}

void ssl_close_connection(SSL *ssl)
{
    if (!ssl) return;

    SSL_shutdown(ssl);
    SSL_free(ssl);
}