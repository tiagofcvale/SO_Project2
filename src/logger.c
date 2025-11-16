#include <stdio.h>
#include <time.h>
#include "logger.h"

void logger_init(void) {
    printf("Logger inicializado\n");
}

void logger_cleanup(void) {
    printf("Logger finalizado\n");
}