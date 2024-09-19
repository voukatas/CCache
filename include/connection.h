#ifndef CLIENT_H
#define CLIENT_H

#include <stdlib.h>
#include "config.h"
#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <sys/epoll.h>
#include "common.h"
#include "hashtable.h"
#include "../include/ttl.h"
#include <stdbool.h>
#include <stdatomic.h>


#ifdef TESTING
//#warning "TESTING macro is defined"
    #define CONNECTIONS_TYPE atomic_int
    #define CONNECTIONS_INIT 0
    #define CONNECTIONS_INCREMENT(var) atomic_fetch_add(&(var), 1)
    #define CONNECTIONS_DECREMENT(var) atomic_fetch_sub(&(var), 1)
    #define CONNECTIONS_GET(var) atomic_load(&(var))
    #define CONNECTIONS_STORE(var, val) atomic_store(&(var), (val))
#else
    #define CONNECTIONS_TYPE int
    #define CONNECTIONS_INIT 0
    #define CONNECTIONS_INCREMENT(var) (var++)
    #define CONNECTIONS_DECREMENT(var) (var--)
    #define CONNECTIONS_GET(var) (var)
    #define CONNECTIONS_STORE(var, val) (var = (val))
#endif

void remove_client_from_list(node_data_t *client_data);
void handle_client_read(client_t *client, struct epoll_event *ev, int epoll_fd);
void handle_client_write(client_t *client, struct epoll_event *ev, int epoll_fd);
void delete_resources(int epoll_fd, client_t *client, struct epoll_event *ev);
void custom_cleanup(void *arg);
bool is_entry_expired(ttl_entry_t *entry, time_t current_time);


void increment_active_connections(void);
void decrement_active_connections(void);
int get_active_connections(void);

extern CONNECTIONS_TYPE active_connections;
extern hash_table_t *hash_table_main;

#endif // CLIENT_H
