#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <errno.h>
#include <pthread.h>

#include "config.h"
#include "shared_mem.h"
#include "semaphores.h"
#include "worker.h"
#include "logger.h"
#include "cache.h"
#include "ssl.h"
#include "global.h"
#include "fd_passing.h"

static volatile int keep_running = 1;
static int current_worker = 0;  // Round-robin distribution

static int create_listener(int port) {
    int fd = socket(AF_INET, SOCK_STREAM, 0);
    if (fd < 0) {
        perror("socket");
        exit(1);
    }

    int yes = 1;
    setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes));

    struct sockaddr_in addr = {0};
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(port);

    if (bind(fd, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        perror("bind");
        exit(1);
    }

    if (listen(fd, 256) < 0) {
        perror("listen");
        exit(1);
    }

    return fd;
}

// PRODUTOR: distribui conexão para próximo worker (round-robin)
static void distribute_connection(int client_fd, int is_https, 
                                  const char *ip, int port) {
    fd_metadata_t meta = {0};
    meta.is_https = is_https;
    strncpy(meta.client_ip, ip, sizeof(meta.client_ip) - 1);
    meta.client_port = port;
    
    // Get next worker socket (round-robin)
    int worker_sock = shm_data->worker_sockets[current_worker][0];
    
    printf("[MASTER] Sending fd=%d (%s) to Worker %d\n", 
           client_fd, is_https ? "HTTPS" : "HTTP", current_worker);
    
    if (send_fd(worker_sock, client_fd, &meta) < 0) {
        fprintf(stderr, "[MASTER] Failed to send FD to worker %d\n", current_worker);
        close(client_fd);
        return;
    }
    
    // Close FD in master (worker now owns it)
    close(client_fd);
    
    // Round-robin to next worker
    current_worker = (current_worker + 1) % shm_data->num_workers;
}

// Thread PRODUTORA para HTTP
static void *producer_thread_http(void *arg) {
    int listen_fd = *(int*)arg;
    
    struct sockaddr_in client_addr;
    socklen_t len = sizeof(client_addr);
    
    printf("[PRODUCER HTTP] Thread started\n");
    
    while (keep_running) {
        int client_fd = accept(listen_fd, (struct sockaddr *)&client_addr, &len);
        
        if (client_fd < 0) {
            if (errno == EINTR) continue;
            perror("accept HTTP");
            continue;
        }
        
        char *ip = inet_ntoa(client_addr.sin_addr);
        int port = ntohs(client_addr.sin_port);
        
        printf("[PRODUCER HTTP] Accepted: fd=%d from %s:%d\n", client_fd, ip, port);
        
        distribute_connection(client_fd, 0, ip, port);
    }
    
    return NULL;
}

// Thread PRODUTORA para HTTPS
static void *producer_thread_https(void *arg) {
    int listen_fd = *(int*)arg;
    
    struct sockaddr_in client_addr;
    socklen_t len = sizeof(client_addr);
    
    printf("[PRODUCER HTTPS] Thread started\n");
    
    while (keep_running) {
        int client_fd = accept(listen_fd, (struct sockaddr *)&client_addr, &len);
        
        if (client_fd < 0) {
            if (errno == EINTR) continue;
            perror("accept HTTPS");
            continue;
        }
        
        char *ip = inet_ntoa(client_addr.sin_addr);
        int port = ntohs(client_addr.sin_port);
        
        printf("[PRODUCER HTTPS] Accepted: fd=%d from %s:%d\n", client_fd, ip, port);
        
        distribute_connection(client_fd, 1, ip, port);
    }
    
    return NULL;
}

// Lança workers CONSUMIDORES
static void launch_workers(void) {
    int n = get_num_workers();
    
    printf("[MASTER] Launching %d consumer workers...\n", n);
    
    for (int i = 0; i < n; i++) {
        // Create Unix socket pair for this worker
        if (create_fd_passing_pair(shm_data->worker_sockets[i]) < 0) {
            perror("socketpair");
            exit(1);
        }
        
        pid_t pid = fork();
        
        if (pid < 0) {
            perror("fork");
            exit(1);
        }
        
        if (pid == 0) {
            // CHILD = WORKER CONSUMIDOR
            // Close master's ends of all sockets
            for (int j = 0; j <= i; j++) {
                close(shm_data->worker_sockets[j][0]);
            }
            
            // This worker uses socket[i][1]
            worker_main(shm_data->worker_sockets[i][1]);
            exit(0);
        }
        
        // PARENT = MASTER
        // Close worker's end
        close(shm_data->worker_sockets[i][1]);
    }
    
    shm_data->num_workers = n;
}

static void sigint_handler(int sig) {
    (void)sig;
    printf("\n[MASTER] Shutting down...\n");
    keep_running = 0;
}

int master_start(void) {
    if (load_config("server.conf") < 0) {
        fprintf(stderr, "Error reading server.conf\n");
        return 1;
    }

    int http_port = get_server_port();
    int https_port = get_https_port();

    printf("[MASTER] ==========================================\n");
    printf("[MASTER] Web Server - PRODUCER-CONSUMER Model\n");
    printf("[MASTER] ==========================================\n");

    // 1) Initialize logger and cache
    logger_init();
    cache_init(get_cache_size_mb());

    // 2) Create shared memory
    shm_data = shm_create_master();
    if (!shm_data) {
        fprintf(stderr, "[MASTER] Failed to create shared memory\n");
        return 1;
    }

    // 3) Initialize semaphores
    if (sem_init_ipc(&sems, CONN_QUEUE_SIZE) != 0) {
        fprintf(stderr, "[MASTER] Failed to initialize semaphores\n");
        return 1;
    }

    // 4) Load SSL (before forking)
    const char *cert = get_ssl_cert();
    const char *key = get_ssl_key();

    global_ssl_ctx = ssl_server_init(cert, key);
    if (!global_ssl_ctx) {
        fprintf(stderr, "[MASTER] WARNING: SSL initialization failed\n");
    } else {
        printf("[MASTER] SSL initialized successfully\n");
    }

    // 5) Launch consumer workers (creates socketpairs)
    launch_workers();

    // 6) Create listening sockets (AFTER fork, only master needs these)
    int listen_http = create_listener(http_port);
    int listen_https = create_listener(https_port);

    printf("[MASTER] HTTP  listening on 0.0.0.0:%d\n", http_port);
    printf("[MASTER] HTTPS listening on 0.0.0.0:%d\n", https_port);

    // 7) Start producer threads
    pthread_t thread_http, thread_https;
    
    pthread_create(&thread_http, NULL, producer_thread_http, &listen_http);
    pthread_create(&thread_https, NULL, producer_thread_https, &listen_https);

    signal(SIGINT, sigint_handler);
    signal(SIGTERM, sigint_handler);

    printf("[MASTER] Server running (PRODUCER mode)\n");
    printf("[MASTER] Workers: %d | Threads/worker: %d\n", 
           get_num_workers(), get_threads_per_worker());
    printf("[MASTER] Press CTRL+C to stop.\n");
    printf("[MASTER] ==========================================\n");

    // Wait for producer threads
    pthread_join(thread_http, NULL);
    pthread_join(thread_https, NULL);

    // Wait for workers
    while (wait(NULL) > 0);

    printf("[MASTER] Cleaning up resources...\n");

    close(listen_http);
    close(listen_https);

    logger_cleanup();
    cache_cleanup();
    sem_cleanup_ipc(&sems);
    shm_destroy(shm_data);

    if (global_ssl_ctx)
        ssl_server_cleanup(global_ssl_ctx);

    printf("[MASTER] Server terminated.\n");
    return 0;
}