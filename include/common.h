#ifndef COMMON_H
#define COMMON_H

#include "config.h"
#include <unistd.h>
#include "logger.h"
#include "hashtable.h"

typedef enum {
    EVENT_SERVER,
    EVENT_CLIENT,
    EVENT_TIMER,

} EventType;

typedef struct {
    int fd;
    char read_buffer[BUFFER_SIZE];
    int read_buffer_len;
    char write_buffer[BUFFER_SIZE];
    int write_buffer_len;
    int write_buffer_pos;
    int close_after_write;
} client_t;

typedef struct {
    int event_type; // 0 for server, 1 for client and 2 for timer
    union {
            int fd;    // Use this if server or timer
            client_t *client; // Use this if client
    } data;
} node_data_t;


extern hash_table_t *hash_table_main;

#endif // COMMON_H
