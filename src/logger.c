#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>

#include "logger.h"
#include "config.h"

static FILE *log_fp = NULL;



/**
 * @brief Inicializa o sistema de logging, abrindo o ficheiro de log para escrita.
 *        Se falhar, usa stdout como fallback.
 */
void logger_init(void) {

    const char *logfile = get_log_file();

    log_fp = fopen(logfile, "a");
    if (!log_fp) {
        perror("logger_init fopen");
        log_fp = stdout;  // fallback
    }

    fprintf(log_fp, "===== Servidor iniciado =====\n");
    fflush(log_fp);
}



/**
 * @brief Regista um evento/acesso no log do servidor.
 * @param ip Endereço IP do cliente.
 * @param method Método HTTP utilizado (ex: GET, POST).
 * @param path Caminho do recurso pedido.
 * @param status Código de estado HTTP da resposta.
 * @param size Número de bytes transferidos na resposta.
 */
void logger_log(const char *ip,
                const char *method,
                const char *path,
                int status,
                long size)
{
    if (!log_fp) return;

    time_t now = time(NULL);
    struct tm *tm_info = localtime(&now);

    char tbuf[32];
    strftime(tbuf, sizeof(tbuf), "%Y-%m-%d %H:%M:%S", tm_info);

    fprintf(log_fp, "[%s] %s \"%s %s\" %d %ld\n",
            tbuf, ip, method, path, status, size);

    fflush(log_fp);
}



/**
 * @brief Finaliza o sistema de logging, fechando o ficheiro de log se necessário.
 */
void logger_cleanup(void) {
    if (log_fp && log_fp != stdout) {
        fprintf(log_fp, "===== Servidor desligado =====\n");
        fclose(log_fp);
    }
}
