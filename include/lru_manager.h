#ifndef LRU_MANAGER_H
#define LRU_MANAGER_H

#include "hashtable.h"

typedef struct lru_entry {
    char* key;
    void *value;
    struct lru_entry *previous;
    struct lru_entry *next;

} lru_entry_t;

typedef struct lru_manager {
    hash_table_t *hash_table_main;
    long size;
    long capacity;
    lru_entry_t *head;
    lru_entry_t *tail;

} lru_manager_t;


void custom_cleanup_lru(void *arg);
void lru_set(char *key, char *value, char *response);
void lru_get(char *key, char *response);
void lru_delete(char *key, char *response);

extern lru_manager_t* lru_manager;

#endif
