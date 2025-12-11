#ifndef CONFIG_H
#define CONFIG_H

// ------------------------------------------------------------
// Server configuration structure
// ------------------------------------------------------------
typedef struct {
    int port;
    int https_port;
    char document_root[256];
    int num_workers;
    int threads_per_worker;
    int max_queue_size;
    char log_file[256];
    int cache_size_mb;
    int timeout_seconds;
    char ssl_cert[256];
    char ssl_key[256];
} server_config_t;


// ------------------------------------------------------------
// API
// ------------------------------------------------------------

// Load configuration from server.conf file
int load_config(const char *filename);

// Get pointer to the entire structure
const server_config_t *get_config(void);

// Individual getters
int get_server_port(void);
int get_https_port(void);
const char *get_document_root(void);
int get_num_workers(void);
int get_threads_per_worker(void);
int get_max_queue_size(void);
const char *get_log_file(void);
int get_cache_size_mb(void);
int get_timeout_seconds(void);
const char *get_ssl_cert(void);
const char *get_ssl_key(void);

#endif