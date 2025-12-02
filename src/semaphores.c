#include <stdio.h>
#include <fcntl.h>
#include <semaphore.h>
#include <errno.h>
#include "semaphores.h"

/**
 * @brief Inicializa e cria os semáforos POSIX nomeados usados pelo servidor.
 *        Remove semáforos antigos, cria novos e associa-os à estrutura fornecida.
 * @param sems Ponteiro para a estrutura onde ficam os descritores dos semáforos.
 * @param queue_size Tamanho da fila de pedidos (valor inicial do semáforo empty).
 * @return 0 em caso de sucesso, -1 em caso de erro.
 */
int sem_init_ipc(ipc_semaphores_t *sems, int queue_size) {
    // Limpeza preventiva de execuções anteriores
    sem_unlink("/sem_ws_empty");
    sem_unlink("/sem_ws_full");
    sem_unlink("/sem_ws_mutex");
    sem_unlink("/sem_ws_stats");
    sem_unlink("/sem_ws_log");

    // Abrir semáforos com O_CREAT
    // Permissões 0666 para leitura/escrita
    
    // sem_empty inicia com queue_size (fila toda vazia)
    sems->sem_empty = sem_open("/sem_ws_empty", O_CREAT, 0666, queue_size);
    
    // sem_full inicia com 0 (nenhum item na fila)
    sems->sem_full  = sem_open("/sem_ws_full",  O_CREAT, 0666, 0);
    
    // Mutexes iniciam com 1 (livres)
    sems->sem_mutex = sem_open("/sem_ws_mutex", O_CREAT, 0666, 1);
    sems->sem_stats = sem_open("/sem_ws_stats", O_CREAT, 0666, 1);
    sems->sem_log   = sem_open("/sem_ws_log",   O_CREAT, 0666, 1);

    // Verificar erros
    if (sems->sem_empty == SEM_FAILED || sems->sem_full == SEM_FAILED || 
        sems->sem_mutex == SEM_FAILED || sems->sem_stats == SEM_FAILED ||
        sems->sem_log == SEM_FAILED) {
        perror("sem_init_ipc falhou");
        return -1;
    }
    return 0;
}

/**
 * @brief Fecha e remove os semáforos POSIX nomeados usados pelo servidor.
 *        Fecha os descritores e faz unlink dos semáforos do sistema operativo.
 * @param sems Ponteiro para a estrutura com os descritores dos semáforos.
 */
void sem_cleanup_ipc(ipc_semaphores_t *sems) {
    // Fechar descritores
    if(sems->sem_empty != SEM_FAILED) sem_close(sems->sem_empty);
    if(sems->sem_full  != SEM_FAILED) sem_close(sems->sem_full);
    if(sems->sem_mutex != SEM_FAILED) sem_close(sems->sem_mutex);
    if(sems->sem_stats != SEM_FAILED) sem_close(sems->sem_stats);
    if(sems->sem_log   != SEM_FAILED) sem_close(sems->sem_log);

    // Remover do sistema operativo (unlink)
    sem_unlink("/sem_ws_empty");
    sem_unlink("/sem_ws_full");
    sem_unlink("/sem_ws_mutex");
    sem_unlink("/sem_ws_stats");
    sem_unlink("/sem_ws_log");
}