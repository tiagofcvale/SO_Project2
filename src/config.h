#ifndef CONFIG_H
#define CONFIG_H

// ------------------------------------------------------------
// Estrutura da configuração do servidor
// ------------------------------------------------------------
typedef struct {
    int port;
    char document_root[256];
    int num_workers;
    int threads_per_worker;
    int max_queue_size;
    char log_file[256];
    int cache_size_mb;
    int timeout_seconds;
} server_config_t;


// ------------------------------------------------------------
// API
// ------------------------------------------------------------

// Carregar configurações do ficheiro server.conf
int load_config(const char *filename);

// Obter ponteiro para toda a estrutura
const server_config_t *get_config(void);

// Getters individuais
int get_server_port(void);
const char *get_document_root(void);
int get_num_workers(void);
int get_threads_per_worker(void);
int get_max_queue_size(void);
const char *get_log_file(void);
int get_cache_size_mb(void);
int get_timeout_seconds(void);

#endif
