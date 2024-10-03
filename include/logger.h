#ifndef LOGGER_H
#define LOGGER_H

#include <stdlib.h>

// Color definitions
#define ANSI_COLOR_RED     "\033[31m"
#define ANSI_COLOR_GREEN   "\033[32m"
#define ANSI_COLOR_YELLOW  "\033[33m"
#define ANSI_COLOR_BLUE    "\033[34m"
#define ANSI_COLOR_MAGENTA "\033[35m"
#define ANSI_COLOR_CYAN    "\033[36m"
#define ANSI_COLOR_RESET   "\033[0m"

#include "log_levels.h"

#define LOG_INFO(fmt, ...)  prod_log(LOG_LEVEL_INFO, "INFO", ANSI_COLOR_GREEN, fmt, ##__VA_ARGS__)
#define LOG_WARN(fmt, ...)  prod_log(LOG_LEVEL_WARN, "WARN", ANSI_COLOR_YELLOW, fmt, ##__VA_ARGS__)
#define LOG_ERROR(fmt, ...) prod_log(LOG_LEVEL_ERROR, "ERROR", ANSI_COLOR_RED, fmt, ##__VA_ARGS__)


#ifdef PDEBUG
    #define DEBUG_LOG(fmt, ...) do { \
        char time_string[40]; \
        get_current_time(time_string, sizeof(time_string)); \
        printf(ANSI_COLOR_CYAN "[%s] [DEBUG] " fmt ANSI_COLOR_RESET, time_string, ##__VA_ARGS__); \
    } while (0)
#else
    #define DEBUG_LOG(fmt, ...) // Do nothing
#endif

// Functions
void prod_log(int level, const char* level_str, const char* color,
              const char* format, ...);
void get_current_time(char* buffer, size_t buffer_size);

// Vars
extern int CURRENT_LOG_LEVEL;

#endif // LOGGER_H

