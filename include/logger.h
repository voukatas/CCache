#ifndef LOGGER_H
#define LOGGER_H

#include <stdlib.h>
#include "log_levels.h"

// Color definitions
#define ANSI_COLOR_RED     "\033[31m"
#define ANSI_COLOR_GREEN   "\033[32m"
#define ANSI_COLOR_YELLOW  "\033[33m"
#define ANSI_COLOR_BLUE    "\033[34m"
#define ANSI_COLOR_MAGENTA "\033[35m"
#define ANSI_COLOR_CYAN    "\033[36m"
#define ANSI_COLOR_RESET   "\033[0m"

// Functions
void log_info(const char* format, ...);
void log_warn(const char* format, ...);
void log_error(const char* format, ...);
void log_debug(const char* format, ...);

// Vars
extern log_level_t CURRENT_LOG_LEVEL;

#endif // LOGGER_H

