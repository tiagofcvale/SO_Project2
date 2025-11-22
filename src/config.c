#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include "config.h"

// ------------------------------------------------------------
// Configuração por defeito
// ------------------------------------------------------------
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


// ------------------------------------------------------------
// trim() — remover espaços
// ------------------------------------------------------------
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


// ------------------------------------------------------------
// load_config() — ler ficheiro server.conf
// ------------------------------------------------------------
int load_config(const char *filename) {

    FILE *file = fopen(filename, "r");
    if (!file) {
        printf("Config file '%s' não encontrado, a usar defaults.\n", filename);
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
            printf("Linha %d inválida: %s\n", line_num, t);
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
            printf("Opção desconhecida na linha %d: %s\n", line_num, key);
    }

    fclose(file);
    return 0;
}


// ------------------------------------------------------------
// Getters
// ------------------------------------------------------------
int get_server_port(void) {
    return config.port;
}

const char *get_document_root(void) {
    return config.document_root;
}

int get_num_workers(void) {
    return config.num_workers;
}

int get_threads_per_worker(void) {
    return config.threads_per_worker;
}

int get_max_queue_size(void) {
    return config.max_queue_size;
}

const char *get_log_file(void) {
    return config.log_file;
}

int get_cache_size_mb(void) {
    return config.cache_size_mb;
}

int get_timeout_seconds(void) {
    return config.timeout_seconds;
}
