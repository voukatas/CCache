#include "../include/logger.h"

#include <stdarg.h>
#include <stdio.h>
#include <time.h>

void get_current_time(char* buffer, size_t buffer_size) {
    time_t now = time(NULL);
    struct tm* t = localtime(&now);
    strftime(buffer, buffer_size, "%Y-%m-%d %H:%M:%S", t);
}

void prod_log(int level, const char* level_str, const char* color,
              const char* format, ...) {
    if (level >= CURRENT_LOG_LEVEL) {
        char time_buffer[20];
        get_current_time(time_buffer, sizeof(time_buffer));

        printf("%s[%s] %s: ", color, time_buffer, level_str);

        va_list args;
        va_start(args, format);
        vprintf(format, args);
        va_end(args);

        printf("%s\n", ANSI_COLOR_RESET);
    }
}

// Consider switching to enums
// typedef enum {
//     LOG_LEVEL_INFO,
//     LOG_LEVEL_WARN,
//     LOG_LEVEL_ERROR
// } log_level_t;
