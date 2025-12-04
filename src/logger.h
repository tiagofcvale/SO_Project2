#ifndef LOGGER_H
#define LOGGER_H

// Initialize the logger
void logger_init(void);

// Register a log line
void logger_log(const char *ip,
                const char *method,
                const char *path,
                int status,
                long size);

// Close log file
void logger_cleanup(void);

#endif
