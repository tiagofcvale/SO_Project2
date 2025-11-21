#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <time.h>
#include <unistd.h>

#include "logger.h"
#include "config.h"

static FILE *log_file = NULL;
static pthread_mutex_t log_mutex = PTHREAD_MUTEX_INITIALIZER;

// Init Logger (server.conf LOG_FILE)

void logger_init(void) {
    const char *filename = get_log_file();

    log_file = fopen(filename, "a");
    if (!log_file) {
        perror("logger_init: fopen");
        exit(1);
    }
}

// Read an entrance at LOG_FILE

void logger_log(const char *client_ip, const char *method, const char *path,
                int status_code, long content_length)
{
    if (!log_file) return;

    pthread_mutex_lock(&log_mutex);

    // Timestamp
    time_t now = time(NULL);
    struct tm *t = localtime(&now);

    char timestamp[64];
    strftime(timestamp, sizeof(timestamp), "%Y-%m-%d %H:%M:%S", t);

    // Final format
    fprintf(log_file, "%s - [%s] \"%s %s\" %d %ld\n",
            client_ip, timestamp, method, path, status_code, content_length);

    fflush(log_file);

    pthread_mutex_unlock(&log_mutex);
}

// Close file

void logger_cleanup(void) {
    if (log_file)
        fclose(log_file);
}
