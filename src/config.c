#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include "config.h"

// Server configuration
static server_config_t config = {
    .port = 8080,
    .document_root = "www",
    .num_workers = 4,
    .threads_per_worker = 10,
    .max_queue_size = 100,
    .log_file = "access.log",
    .cache_size_mb = 10,
    .timeout_seconds = 30
};



/**
 * @brief Removes whitespace from the beginning and end of a string.
 * @param str String to be processed.
 * @return Pointer to the trimmed string.
 */
static char *trim(char *str) {
    char *end;

    while (isspace((unsigned char)*str))
        str++;

    if (*str == 0)
        return str;

    end = str + strlen(str) - 1;
    while (end > str && isspace((unsigned char)*end))
        end--;

    end[1] = '\0';
    return str;
}



/**
 * @brief Reads the configuration file and updates the server parameters.
 * @param filename Name of the configuration file.
 * @return 0 on success (or file not found), -1 on fatal error.
 */
int load_config(const char *filename) {

    FILE *file = fopen(filename, "r");
    if (!file) {
        printf("Config file '%s' not found, using defaults.\n", filename);
        return 0;
    }

    char line[256];
    int line_num = 0;

    while (fgets(line, sizeof(line), file)) {
        line_num++;

        char *t = trim(line);

        if (t[0] == '#' || t[0] == '\0')
            continue;

        char *value = strchr(t, '=');
        if (!value) {
            printf("Invalid line %d: %s\n", line_num, t);
            continue;
        }

        *value = '\0';
        value++;

        char *key = trim(t);
        value = trim(value);

        if (strcmp(key, "PORT") == 0)
            config.port = atoi(value);

        else if (strcmp(key, "DOCUMENT_ROOT") == 0)
            strncpy(config.document_root, value, sizeof(config.document_root)-1);

        else if (strcmp(key, "NUM_WORKERS") == 0)
            config.num_workers = atoi(value);

        else if (strcmp(key, "THREADS_PER_WORKER") == 0)
            config.threads_per_worker = atoi(value);

        else if (strcmp(key, "MAX_QUEUE_SIZE") == 0)
            config.max_queue_size = atoi(value);

        else if (strcmp(key, "LOG_FILE") == 0)
            strncpy(config.log_file, value, sizeof(config.log_file)-1);

        else if (strcmp(key, "CACHE_SIZE_MB") == 0)
            config.cache_size_mb = atoi(value);

        else if (strcmp(key, "TIMEOUT_SECONDS") == 0)
            config.timeout_seconds = atoi(value);

        else
            printf("Unknown option on line %d: %s\n", line_num, key);
    }

    fclose(file);
    return 0;
}



/**
 * @brief Gets a pointer to the entire configuration structure.
 * @return Constant pointer to the server configuration.
 */
const server_config_t *get_config(void) {
    return &config;
}

/**
 * @brief Gets the server port number.
 * @return Configured TCP port.
 */
int get_server_port(void) {
    return config.port;
}

/**
 * @brief Gets the document root directory to serve.
 * @return String with the document root path.
 */
const char *get_document_root(void) {
    return config.document_root;
}

/**
 * @brief Gets the number of configured worker processes.
 * @return Number of workers.
 */
int get_num_workers(void) {
    return config.num_workers;
}

/**
 * @brief Gets the number of threads per worker.
 * @return Number of threads per worker.
 */
int get_threads_per_worker(void) {
    return config.threads_per_worker;
}

/**
 * @brief Gets the maximum request queue size.
 * @return Queue size.
 */
int get_max_queue_size(void) {
    return config.max_queue_size;
}

/**
 * @brief Gets the log file name.
 * @return String with the log file name.
 */
const char *get_log_file(void) {
    return config.log_file;
}

/**
 * @brief Gets the cache size in megabytes.
 * @return Cache size in MB.
 */
int get_cache_size_mb(void) {
    return config.cache_size_mb;
}

/**
 * @brief Gets the configured timeout for server operations.
 * @return Timeout in seconds.
 */
int get_timeout_seconds(void) {
    return config.timeout_seconds;
}