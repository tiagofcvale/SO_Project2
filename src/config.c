#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "config.h"

static int server_port = 8080; // porta padrão

int load_config(const char *filename) {
    FILE *file = fopen(filename, "r");
    if (!file) {
        printf("Config file not found, using default port %d\n", server_port);
        return 0; // Não é fatal se o arquivo não existir
    }
    
    char line[256];
    while (fgets(line, sizeof(line), file)) {
        if (strncmp(line, "port", 4) == 0) {
            sscanf(line, "port = %d", &server_port);
        }
    }
    
    fclose(file);
    printf("Server port configured to: %d\n", server_port);
    return 0;
}

int get_server_port(void) {
    return server_port;
}