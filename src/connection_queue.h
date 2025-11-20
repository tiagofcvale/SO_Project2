#ifndef CONNECTION_QUEUE_H
#define CONNECTION_QUEUE_H

#define MAX_QUEUE_SIZE 100

typedef struct {
    int sockets[MAX_QUEUE_SIZE];
    int front;
    int rear;
    int count;
} connection_queue_t;

#endif
