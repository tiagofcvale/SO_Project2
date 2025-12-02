#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "config.h"
#include "logger.h"
#include "stats.h"
#include "cache.h"
#include "master.h"

/**
 * @brief Função principal do servidor HTTP. Carrega configuração, inicializa módulos
 *        (logger, cache) e arranca o processo master que gere os workers e recursos IPC.
 * @return 0 em caso de sucesso, 1 em caso de erro crítico.
 */
int main(void) {

    // 1. Carregar configuração
    if (load_config("server.conf") != 0) {
        fprintf(stderr, "Erro ao carregar configuração.\n");
        return 1;
    }

    printf("A iniciar servidor HTTP com:\n");
    printf("- Workers: %d\n", get_num_workers());
    printf("- Threads por worker: %d\n", get_threads_per_worker());
    printf("- Document root: %s\n", get_document_root());
    printf("- Cache: %d MB\n", get_cache_size_mb());

    // 2. Iniciar Logger
    logger_init();

    // 3. Iniciar Cache (Global para o Master, herdada pelos Workers no fork)
    cache_init(get_cache_size_mb());

    // NOTA: stats_init removido daqui.
    // Agora é o master_start() que inicializa as stats dentro da Memória Partilhada.

    // 4. Iniciar o Mestre (Cria SHM, Semáforos, Sockets e Workers)
    if (master_start() != 0) {
        fprintf(stderr, "Erro crítico a iniciar servidor.\n");
        logger_cleanup();
        return 1;
    }

    // Limpeza final (só chega aqui se o master retornar, o que não deve acontecer em loop)
    logger_cleanup();
    return 0;
}