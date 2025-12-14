#ifndef GLOBAL_H
#define GLOBAL_H

#include "shared_mem.h"
#include "semaphores.h"
#include "ssl.h"

// Global variable for debugging
extern int GLOBAL_DEBUG_MODE;

// Shared memory and semaphores (shared across master and workers)
extern shared_data_t* shm_data;
extern ipc_semaphores_t sems;

// SSL context (created in master, inherited by workers)
extern ssl_server_ctx_t *global_ssl_ctx;

#endif
