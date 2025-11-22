#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "config.h"
#include "logger.h"
#include "stats.h"
#include "cache.h"
#include "master.h"

int main(void) {

    // Load config
    if (load_config("server.conf") != 0) {
        fprintf(stderr, "Erro ao carregar configuração.\n");
        return 1;
    }

    printf("A iniciar servidor HTTP com:\n");
    printf("- Workers: %d\n", get_num_workers());
    printf("- Threads por worker: %d\n", get_threads_per_worker());
    printf("- Document root: %s\n", get_document_root());
    printf("- Cache: %d MB\n", get_cache_size_mb());

    // Start logger
    logger_init();

    // Start statistics module
    stats_init(get_num_workers());

    // Start cache
    cache_init(get_cache_size_mb());

    // Start master server loop
    if (master_start() != 0) {
        fprintf(stderr, "Erro a iniciar servidor.\n");
        return 1;
    }

    logger_cleanup();
    return 0;
}
