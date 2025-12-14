#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "config.h"
#include "logger.h"
#include "stats.h"
#include "cache.h"
#include "master.h"

/**
 * @brief Main function of the HTTP server. Loads configuration and starts the master process.
 * @return 0 on success, 1 on critical error.
 */
int main(void) {
    printf("Starting HTTP/HTTPS Server...\n");
    printf("===============================\n");

    // Start the Master (which handles everything else)
    if (master_start() != 0) {
        fprintf(stderr, "Critical error starting server.\n");
        return 1;
    }

    return 0;
}