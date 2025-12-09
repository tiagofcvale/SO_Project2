#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>

#define NUM_THREADS 10  // Number of threads to test concurrency

// Simulated server request function (simple example)
void *server_request(void *thread_id) {
    long tid = (long)thread_id;
    printf("Thread %ld: Sending request to server...\n", tid);

    // Simulating server response time
    sleep(1);

    // Here you would call the server or the function you want to test
    printf("Thread %ld: Response received from server\n", tid);
    
    pthread_exit(NULL);
}

int main() {
    pthread_t threads[NUM_THREADS];
    int rc;
    
    // Creating the threads
    for (long t = 0; t < NUM_THREADS; t++) {
        printf("Main: creating thread %ld\n", t);
        rc = pthread_create(&threads[t], NULL, server_request, (void *)t);
        if (rc) {
            printf("Error creating thread %ld, error code: %d\n", t, rc);
            exit(-1);
        }
    }

    // Wait for all threads to finish
    for (long t = 0; t < NUM_THREADS; t++) {
        pthread_join(threads[t], NULL);
    }

    printf("All threads have finished.\n");

    return 0;
}
