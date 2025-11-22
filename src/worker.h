#ifndef WORKER_H
#define WORKER_H

// Cada worker recebe o listen_fd (socket de escuta) do master
void worker_main(int listen_fd);

#endif
