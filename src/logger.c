#include "../include/logger.h"

#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <sys/time.h>
#include <time.h>

static void get_current_time(char* buffer, size_t buffer_size) {
    time_t now = time(NULL);
    struct tm* t = localtime(&now);
    strftime(buffer, buffer_size, "%Y-%m-%d %H:%M:%S", t);
    // Add milliseconds
    struct timeval tv;
    gettimeofday(&tv, NULL);
    size_t len = strlen(buffer);
    snprintf(buffer + len, buffer_size - len, ".%03ld", tv.tv_usec / 1000);
}

static void prod_log(log_level_t level, const char* level_str,
                     const char* color, const char* format, va_list args) {
    if (level >= CURRENT_LOG_LEVEL) {
        char time_buffer[30];
        get_current_time(time_buffer, sizeof(time_buffer));

        printf("%s[%s] %s: ", color, time_buffer, level_str);
        vprintf(format, args);
        printf("%s\n", ANSI_COLOR_RESET);
    }
}

void log_info(const char* format, ...) {
    va_list args;
    va_start(args, format);
    prod_log(LOG_LEVEL_INFO, "INFO", ANSI_COLOR_GREEN, format, args);
    va_end(args);
}

void log_warn(const char* format, ...) {
    va_list args;
    va_start(args, format);
    prod_log(LOG_LEVEL_WARN, "WARN", ANSI_COLOR_YELLOW, format, args);
    va_end(args);
}

void log_error(const char* format, ...) {
    va_list args;
    va_start(args, format);
    prod_log(LOG_LEVEL_ERROR, "ERROR", ANSI_COLOR_RED, format, args);
    va_end(args);
}

void log_debug(const char* format, ...) {
    va_list args;
    va_start(args, format);
    prod_log(LOG_LEVEL_DEBUG, "DEBUG", ANSI_COLOR_CYAN, format, args);
    va_end(args);
}
