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
    
    printf("A iniciar servidor HTTP com:\n");
    printf("- Workers: %d\n", get_num_workers());
    printf("- Threads por worker: %d\n", get_threads_per_worker());
    printf("- Document root: %s\n", get_document_root());
    printf("- Cache: %d MB\n", get_cache_size_mb());
    
    // Iniciar servidor master
    if (master_start() != 0) {
        fprintf(stderr, "Erro ao iniciar servidor\n");
        logger_cleanup();
        return 1;
    }
    
    logger_cleanup();
    return 0;
}