#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>

#include "shared_mem.h"

shared_data_t* shm_create_master(void) {
    shm_unlink(SHM_NAME);
    
    int fd = shm_open(SHM_NAME, O_CREAT | O_EXCL | O_RDWR, 0666);
    if (fd == -1) {
        perror("shm_open (master)");
        return NULL;
    }

    if (ftruncate(fd, sizeof(shared_data_t)) == -1) {
        perror("ftruncate");
        close(fd);
        shm_unlink(SHM_NAME);
        return NULL;
    }

    shared_data_t *ptr = mmap(NULL, sizeof(shared_data_t),
                              PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    close(fd);

    if (ptr == MAP_FAILED) {
        perror("mmap");
        shm_unlink(SHM_NAME);
        return NULL;
    }
    
    // Initialize structure
    memset(ptr, 0, sizeof(shared_data_t));
    
    printf("[SHM] Created shared memory\n");
    
    return ptr;
}

shared_data_t* shm_attach_worker(void) {
    int fd = shm_open(SHM_NAME, O_RDWR, 0666);
    if (fd == -1) {
        perror("shm_open (worker)");
        return NULL;
    }

    shared_data_t *ptr = mmap(NULL, sizeof(shared_data_t),
                              PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    close(fd);

    if (ptr == MAP_FAILED) {
        perror("mmap (worker)");
        return NULL;
    }
    
    return ptr;
}

void shm_destroy(shared_data_t* data) {
    if (data) {
        munmap(data, sizeof(shared_data_t));
    }
    shm_unlink(SHM_NAME);
}