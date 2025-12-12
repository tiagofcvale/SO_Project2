#ifndef MASTER_H
#define MASTER_H

// Starts the master server (socket, workers, SSL)
int master_start(void);

// Stops and cleans up server resources
int master_stop(void);

#endif