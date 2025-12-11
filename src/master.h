#ifndef MASTER_H
#define MASTER_H

// Inicia o servidor master (socket, workers, SSL)
int master_start(void);

// Para e limpa recursos do servidor
int master_stop(void);

#endif