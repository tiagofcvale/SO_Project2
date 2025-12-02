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
 * @brief Cria ou abre um objeto de memória partilhada POSIX e faz o seu mapeamento.
 * @return Ponteiro para a memória partilhada mapeada, ou NULL em caso de erro.
 */
shared_data_t* shm_create(void) {
    // 1. Criar/Abrir objeto de memória partilhada POSIX
    int fd = shm_open(SHM_NAME, O_CREAT | O_RDWR, 0666);
    if (fd == -1) {
        perror("shm_open falhou");
        return NULL;
    }

    // 2. Definir o tamanho exato da estrutura
    if (ftruncate(fd, sizeof(shared_data_t)) == -1) {
        perror("ftruncate falhou");
        close(fd);
        return NULL;
    }

    // 3. Mapear na memória do processo
    shared_data_t *ptr = mmap(NULL, sizeof(shared_data_t),
                              PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    
    // O descritor já não é necessário após o mmap
    close(fd);

    if (ptr == MAP_FAILED) {
        perror("mmap falhou");
        return NULL;
    }
    
    return ptr;
}

/**
 * @brief Liberta o mapeamento da memória partilhada e remove o objeto do sistema operativo.
 * @param data Ponteiro para a memória partilhada a desmapear.
 */
void shm_destroy(shared_data_t* data) {
    if (data) {
        // Desmapear a memória do processo
        munmap(data, sizeof(shared_data_t));
    }
    // Remover o objeto do sistema operativo (importante!)
    shm_unlink(SHM_NAME);
}