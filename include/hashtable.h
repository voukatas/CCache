#ifndef HASHTABLE_H
#define HASHTABLE_H


#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>

// Entry struct
typedef struct hash_entry {
    char *key;
    void *value;
    struct hash_entry *next;

} hash_entry_t;

// HashTable
typedef struct hash_table {
    int capacity;
    int size;
    hash_entry_t **table;
    //pthread_mutex_t hash_table_mutex;
} hash_table_t;

hash_table_t *hash_table_create(int capacity);
//int hash_table_set(hash_table_t *ht, char *key, void *value, size_t size);
int hash_table_set(hash_table_t *ht, char *key, void *value, size_t size, void (*cleanup_callback)(void *));
void *hash_table_get(hash_table_t *ht, char *key);
void hash_table_print_keys(hash_table_t *ht);
void hash_table_cleanup(hash_table_t *ht, void (*cleanup_callback)(void *));
int hash_table_remove(hash_table_t *ht, char *key, void (*cleanup_callback)(void *));
void hash_table_set_resize_flag(int enabled);

// Consider indirect testing for this so I can make it static
#ifdef TESTING
int hash_table_resize(hash_table_t *ht);
#endif

// Features flags
extern int hash_table_resize_fc_enabled;

#endif
