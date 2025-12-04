#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "config.h"
#include "logger.h"
#include "stats.h"
#include "cache.h"
#include "master.h"


/**
 * @brief Main function of the HTTP server. Loads configuration, initializes modules
 *        (logger, cache), and starts the master process that manages workers and IPC resources.
 * @return 0 on success, 1 on critical error.
 */
int main(void) {

    // 1. Load configuration
    if (load_config("server.conf") != 0) {
        fprintf(stderr, "Error loading configuration.\n");
        return 1;
    }

    printf("Starting HTTP server with:\n");
    printf("- Workers: %d\n", get_num_workers());
    printf("- Threads per worker: %d\n", get_threads_per_worker());
    printf("- Document root: %s\n", get_document_root());
    printf("- Cache: %d MB\n", get_cache_size_mb());

    // 2. Start Logger
    logger_init();

    // 3. Start Cache (Global for the Master, inherited by Workers on fork)
    cache_init(get_cache_size_mb());

    // NOTE: stats_init removed from here.
    // Now master_start() initializes stats inside Shared Memory.

    // 4. Start the Master (Creates SHM, Semaphores, Sockets, and Workers)
    if (master_start() != 0) {
        fprintf(stderr, "Critical error starting server.\n");
        cache_cleanup();
        logger_cleanup();
        return 1;
    }

    // Final cleanup (only reached if master returns, which should not happen in loop)
    cache_cleanup();
    logger_cleanup();
    return 0;
}