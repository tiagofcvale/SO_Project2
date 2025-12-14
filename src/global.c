#include "global.h"
#include "shared_mem.h"
#include "semaphores.h"
#include "ssl.h"

// Global variables

int GLOBAL_DEBUG_MODE = 0;

// Variáveis globais para shared memory e semáforos
shared_data_t* shm_data = NULL;
ipc_semaphores_t sems;

// Contexto SSL global
ssl_server_ctx_t *global_ssl_ctx = NULL;
