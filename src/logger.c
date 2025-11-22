#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>

#include "logger.h"
#include "config.h"

static FILE *log_fp = NULL;


// ------------------------------------------------------------
// Inicializar logger
// ------------------------------------------------------------
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


// ------------------------------------------------------------
// Registar um evento no log
// ------------------------------------------------------------
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


// ------------------------------------------------------------
// Finalizar logger
// ------------------------------------------------------------
void logger_cleanup(void) {
    if (log_fp && log_fp != stdout) {
        fprintf(log_fp, "===== Servidor desligado =====\n");
        fclose(log_fp);
    }
}
