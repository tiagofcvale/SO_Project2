#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>

#include "logger.h"
#include "config.h"

static FILE *log_fp = NULL;



/**
 * @brief Initializes the logging system, opening the log file for writing.
 *        If it fails, uses stdout as a fallback.
 */
void logger_init(void) {

    const char *logfile = get_log_file();

    log_fp = fopen(logfile, "a");
    if (!log_fp) {
        perror("logger_init fopen");
        log_fp = stdout;  // fallback
    }

    fprintf(log_fp, "===== Server on =====\n");
    fflush(log_fp);
}



/**
 * @brief Logs an event/access in the server log.
 * @param ip Client IP address.
 * @param method HTTP method used (e.g., GET, POST).
 * @param path Path of the requested resource.
 * @param status HTTP status code of the response.
 * @param size Number of bytes transferred in the response.
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
 * @brief Finalizes the logging system, closing the log file if necessary.
 */
void logger_cleanup(void) {
    if (log_fp && log_fp != stdout) {
        fprintf(log_fp, "===== Server off =====\n");
        fclose(log_fp);
    }
}
