#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>
#include <signal.h>
#include <errno.h>
#include <fcntl.h>
#include <time.h>


#include "master.h"
#include "config.h"
#include "logger.h"
#include "worker.h"
#include "shared_mem.h"
#include "semaphores.h"

// Global variables for cleanup in the signal handler
static int server_socket = -1;
static shared_data_t *shm_data = NULL;
static ipc_semaphores_t sems;
static pid_t *worker_pids = NULL;


/**
 * @brief Sends an HTTP 503 response and closes the socket, used when the queue is full.
 * @param fd Client socket descriptor.
 */
void send_503_and_close(int fd) {
    const char *resp = "HTTP/1.1 503 Service Unavailable\r\n"
                       "Content-Type: text/plain\r\n"
                       "Connection: close\r\n\r\n"
                       "Server too busy, try again later.\n";
    send(fd, resp, strlen(resp), 0);
    close(fd);
}

/**
 * @brief Handler for the SIGINT signal (CTRL+C). Cleans up IPC resources, terminates workers, and shuts down the server.
 * @param sig Signal number received (unused).
 */
void handle_sigint(int sig) {
    static int already_cleaned = 0;
    if (already_cleaned) {
        return;
    }
    already_cleaned = 1;
    (void)sig;
    printf("\n[Master] Received SIGINT. Shutting down server...\n");

    // 1. Kill worker processes
    int n = get_num_workers();
    if (worker_pids) {
        for (int i = 0; i < n; i++) {
            if (worker_pids[i] > 0) {
                kill(worker_pids[i], SIGTERM);
            }
        }
        free(worker_pids);
    }

    // 2. Close main socket
    if (server_socket >= 0) {
        close(server_socket);
    }

    // 3. Clean up IPC (CRUCIAL: shm_unlink and sem_unlink)
    shm_destroy(shm_data);
    sem_cleanup_ipc(&sems);

    logger_cleanup();
    printf("[Master] Clean up is done. Bye.\n");
    exit(0);
}


/**
 * @brief Main function of the master process. Initializes IPC, socket, creates workers, and monitors the server.
 * @return 0 on success, -1 on error.
 */
int master_start(void) {
    signal(SIGINT, handle_sigint);

    // 1. Initialize IPC
    shm_data = shm_create();
    if (!shm_data) return -1;
    
    // Initialize Stats and Queue
    stats_init(&shm_data->stats);
    shm_data->queue.front = 0;
    shm_data->queue.rear = 0;
    shm_data->queue.count = 0;

    // Initialize Semaphores
    if (sem_init_ipc(&sems, get_max_queue_size()) < 0) {
        shm_destroy(shm_data);
        return -1;
    }

    // 2. Create Socket
    struct sockaddr_in server_addr;
    int opt = 1;

    server_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (server_socket < 0) { perror("socket"); return -1; }
    setsockopt(server_socket, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(get_server_port());

    if (bind(server_socket, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("bind"); return -1;
    }
    if (listen(server_socket, 128) < 0) {
        perror("listen"); return -1;
    }

    printf("[Master] Listening on port %d. Launching workers...\n", get_server_port());

    // 3. Create Workers
    int n_workers = get_num_workers();
    worker_pids = malloc(sizeof(pid_t) * n_workers);

    for (int i = 0; i < n_workers; i++) {
        pid_t pid = fork();
        if (pid == 0) {
            // Worker: create a new group of processes and ignores SIGINT
            setpgid(0, 0); // Worker stay on its own group
            signal(SIGINT, SIG_IGN); // Ignore SIGINT (CTRL+C)
            
            // Worker INHERITS the server_socket and will use it
            worker_main(server_socket); 
            exit(0);
        } else {
            worker_pids[i] = pid;
        }
    }

    // 4. Master in Monitoring mode
    // The Master no longer does accept, only monitors and prints stats
    printf("[Master] Server On (CTRL+C to off)\n");
    
    while (1) {
        sleep(12); // Print stats every 12 seconds
        stats_print(&shm_data->stats, sems.sem_stats);
    }

    return 0;
}

/**
 * @brief Stops the server in an orderly way (optional function for interface)
 * @return 0 on success
 */
int master_stop(void) {
    // Trigger shutdown via signal
    raise(SIGINT);
    return 0;
}