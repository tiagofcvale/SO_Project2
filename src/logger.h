#ifndef LOGGER_H
#define LOGGER_H

#include <time.h>

void logger_init(void);
void logger_log(const char *client_ip, const char *method, const char *path,
                int status_code, long content_length);
void logger_cleanup(void);

#endif
