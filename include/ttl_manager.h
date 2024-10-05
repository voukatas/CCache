#ifndef TTL_MANAGER_H
#define TTL_MANAGER_H

#include <time.h>
#include "hashtable.h"
#include <errno.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../include/config.h"
#include "../include/logger.h"
#include "../include/common.h"
#include "../include/response.h"

typedef struct ttl_entry {
    void *value;
    time_t timestamp;
    int ttl;
} ttl_entry_t;

typedef struct ttl_manager {
    hash_table_t *hash_table_main;
} ttl_manager_t;


// Functions
void ttl_set(char *key, char *value, char *ttl_value, char *response);
void ttl_get(char *key, char *response);
void ttl_delete(char *key, char *response);
void custom_cleanup_ttl(void *arg);
bool is_entry_expired(ttl_entry_t *entry, time_t current_time);

// Vars
extern ttl_manager_t* ttl_manager;

#endif // TTL_MANAGER_H
