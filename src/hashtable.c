#include <pthread.h>
#include <string.h>
#ifdef TESTING
// #warning "TESTING macro is defined"
#include "../include/mock_malloc.h"
#endif

#include "../include/hashtable.h"

int hash_table_resize(hash_table_t *ht);
int hash_table_resize_fc_enabled = 1;

// pthread_mutex_t hash_table_mutex = PTHREAD_MUTEX_INITIALIZER;
//  init table
hash_table_t *hash_table_create(int capacity) {
    hash_table_t *ht = malloc(sizeof(hash_table_t));
    if (!ht) {
        printf("failed to allocate memory: %s", strerror(errno));
        exit(EXIT_FAILURE);
    }

    ht->table = malloc(sizeof(hash_entry_t *) * capacity);
    if (!ht->table) {
        printf("failed to allocate memory: %s", strerror(errno));
        free(ht);
        exit(EXIT_FAILURE);
    }

    ht->capacity = capacity;
    ht->size = 0;

    for (int i = 0; i < capacity; i++) {
        ht->table[i] = NULL;
    }

    return ht;
}

// Hash djb2
static int hash(char *key, int capacity) {
    unsigned long hash = 5381;
    int c;

    while ((c = *key++)) {
        hash = ((hash << 5) + hash) + c;  // hash * 33 + c
    }

    // printf("address: %d\n", hash% TABLE_SIZE);

    return hash % capacity;
}

// Set
int hash_table_set(hash_table_t *ht, char *key, void *value, size_t size,
                   void (*cleanup_callback)(void *)) {
    if (key == NULL || value == NULL) {
        printf("invalid memory reference\n");
        return -1;
    }
    // printf("init size: %d\n", ht->size);
    if ((ht->size >= (ht->capacity * 0.75)) && hash_table_resize_fc_enabled) {
        // printf("size: %d cap: %d\n", ht->size, ht->capacity / 2);
        int result = hash_table_resize(ht);
        if (result != 0) {
            return -1;
        }
    }
    int address = hash(key, ht->capacity);

    hash_entry_t *entry = ht->table[address];

    while (entry != NULL) {
        if (strcmp(entry->key, key) == 0) {
            // printf("entry key: %s key: %s\n", entry->Key, key);

            void *new_value = malloc(size);

            if (!new_value) {
                printf("failed to allocate memory: %s\n", strerror(errno));
                return -1;
            }
            // free the old value only if the allocation succeeds to
            // prevent data loss
            memcpy(new_value, value, size);

            // check if special handling is needed
            if (cleanup_callback) {
                cleanup_callback(entry->value);
            }
            free(entry->value);

            entry->value = new_value;

            return 0;  // Success
        }
        entry = entry->next;
    }

    entry = malloc(sizeof(hash_entry_t));  // consider also calloc
    if (!entry) {
        printf("failed to allocate memory: %s\n", strerror(errno));
        return -1;
    }
    entry->key = strdup(key);
    // entry->value = strdup(value);
    entry->value = malloc(size);
    if (!entry->key || !entry->value) {
        printf("failed to allocate memory for key or value: %s\n",
               strerror(errno));
        free(entry->key);
        free(entry->value);
        free(entry);
        return -1;  // Error
    }
    memcpy(entry->value, value, size);

    entry->next = ht->table[address];
    ht->table[address] = entry;

    ht->size++;
    // printf("---increase size: %d\n", ht->size);

    return 0;
}

// Get
void *hash_table_get(hash_table_t *ht, char *key) {
    int address = hash(key, ht->capacity);
    hash_entry_t *entry = ht->table[address];

    while (entry != NULL) {
        if (strcmp(entry->key, key) == 0) {
            return entry->value;
        }
        entry = entry->next;
    }

    return NULL;
}

int hash_table_remove(hash_table_t *ht, char *key,
                      void (*cleanup_callback)(void *)) {
    int address = hash(key, ht->capacity);

    hash_entry_t *current_entry = ht->table[address];
    hash_entry_t *prev_entry = NULL;

    while (current_entry != NULL) {
        if (strcmp(current_entry->key, key) == 0) {
            if (prev_entry == NULL) {
                ht->table[address] = current_entry->next;
            } else {
                prev_entry->next = current_entry->next;
            }
            free(current_entry->key);
            // check if special handling is needed
            if (cleanup_callback) {
                cleanup_callback(current_entry->value);
            }
            free(current_entry->value);
            free(current_entry);
            ht->size--;
            return 0;  // Success
        }
        prev_entry = current_entry;
        current_entry = current_entry->next;
    }

    return -1;  // Key not found
}

// resize
// Avoid the use of realloc here because you will end up reading and modifying
// the same table...
int hash_table_resize(hash_table_t *ht) {
    // printf("resize initiated\n");
    int new_capacity_no = 2 * ht->capacity;
    hash_entry_t **new_table =
        (hash_entry_t **)malloc(new_capacity_no * sizeof(hash_entry_t *));
    if (new_table == NULL) {
        printf("failed to allocate memory for new table: %s\n",
               strerror(errno));
        return -1;
    }

    for (int i = 0; i < new_capacity_no; i++) {
        new_table[i] = NULL;
    }

    // Re-hash
    for (int i = 0; i < ht->capacity; i++) {
        hash_entry_t *entry = ht->table[i];
        while (entry != NULL) {
            hash_entry_t *next = entry->next;
            int address = hash(entry->key, new_capacity_no);
            entry->next = new_table[address];
            new_table[address] = entry;
            entry = next;
        }
    }

    free(ht->table);
    ht->table = new_table;
    ht->capacity = new_capacity_no;

    // printf("resize finished\n");
    return 0;
}

// Keys
void hash_table_print_keys(hash_table_t *ht) {
    printf("Keys:\n");
    for (int i = 0; i < ht->capacity; i++) {
        hash_entry_t *entry = ht->table[i];
        if (entry == NULL) {
            continue;
        }

        while (entry != NULL) {
            printf("position: %d key: %s\n", i, entry->key);
            entry = entry->next;
        }
    }
}

// CleanUp
void hash_table_cleanup(hash_table_t *ht, void (*cleanup_callback)(void *)) {
    for (int i = 0; i < ht->capacity; i++) {
        hash_entry_t *entry = ht->table[i];
        if (entry == NULL) {
            continue;
        }
        while (entry != NULL) {
            hash_entry_t *tmp = entry->next;
            // check if special handling is needed
            if (cleanup_callback) {
                cleanup_callback(entry->value);
            }
            free(entry->value);
            free(entry->key);
            free(entry);
            entry = tmp;
        }
    }
    free(ht->table);
    free(ht);
}

void hash_table_set_resize_flag(int enabled) {
    hash_table_resize_fc_enabled = enabled;
}
