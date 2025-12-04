#ifndef WORKER_H
#define WORKER_H

// Each worker receives the listen_fd (listening socket) from the master
void worker_main(int listen_fd);

#endif
