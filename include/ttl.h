#ifndef TTL_H
#define TTL_H

#include <time.h>

typedef struct ttl_entry {
    void *value;
    time_t timestamp;
    int ttl;
} ttl_entry_t;

#endif // TTL_H
