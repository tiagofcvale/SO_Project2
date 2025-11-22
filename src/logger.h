#ifndef LOGGER_H
#define LOGGER_H

// Inicializar o logger
void logger_init(void);

// Registar uma linha de log
void logger_log(const char *ip,
                const char *method,
                const char *path,
                int status,
                long size);

// Fechar ficheiro de log
void logger_cleanup(void);

#endif
