#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "master.h"
#include "config.h"
#include "logger.h"

int main(void) {
    // Carregar configuração
    if (load_config("server.conf") != 0) {
        fprintf(stderr, "Erro ao carregar configuração\n");
        return 1;
    }
    
    // Inicializar logger
    logger_init();
    
    // Iniciar servidor master
    if (master_start() != 0) {
        fprintf(stderr, "Erro ao iniciar servidor\n");
        return 1;
    }
    
    logger_cleanup();
    return 0;
}