#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>

#include "shared_mem.h"

/**
 * @brief Creates or opens a POSIX shared memory object and maps it.
 * @return Pointer to the mapped shared memory, or NULL on error.
 */
shared_data_t* shm_create(void) {
    // 1. Create/Open POSIX shared memory object
    int fd = shm_open(SHM_NAME, O_CREAT | O_RDWR, 0666);
    if (fd == -1) {
        perror("shm_open failed");
        return NULL;
    }

    // 2. Set the exact size of the structure
    if (ftruncate(fd, sizeof(shared_data_t)) == -1) {
        perror("ftruncate failed");
        close(fd);
        return NULL;
    }

    // 3. Map into the process memory
    shared_data_t *ptr = mmap(NULL, sizeof(shared_data_t),
                              PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    
    // The descriptor is no longer needed after mmap
    close(fd);

    if (ptr == MAP_FAILED) {
        perror("mmap failed");
        return NULL;
    }
    
    return ptr;
}

/**
 * @brief Frees the shared memory mapping and removes the object from the operating system.
 * @param data Pointer to the shared memory to unmap.
 */
void shm_destroy(shared_data_t* data) {
    if (data) {
        // Unmap the memory from the process
        munmap(data, sizeof(shared_data_t));
    }
    // Remove the object from the operating system (important!)
    shm_unlink(SHM_NAME);
}