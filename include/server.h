#ifndef SERVER_H
#define SERVER_H

#include "common.h"
#include <stdatomic.h>

typedef enum {
    SERVER_STATE_INITIALIZING,
    SERVER_STATE_RUNNING,
    SERVER_STATE_STOPPED
} server_state_t;

int run_server(int port);

extern atomic_int server_state;
void set_server_state(server_state_t state);
server_state_t get_server_state(void);

#endif // SERVER_H
