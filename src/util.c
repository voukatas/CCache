#include "../include/util.h"

void print_exact_time(char* message) {
    struct timeval tv;
    struct tm* ptm;
    char time_string[40];
    long milliseconds;

    gettimeofday(&tv, NULL);
    ptm = localtime(&tv.tv_sec);
    strftime(time_string, sizeof(time_string), "%Y-%m-%d %H:%M:%S", ptm);
    milliseconds = tv.tv_usec / 1000;

    printf("[ %s.%03ld ] - %s", time_string, milliseconds, message);
}
