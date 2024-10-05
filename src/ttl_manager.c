#include "../include/ttl_manager.h"

// A custom clean function that is used as a callback in the hashtable
void custom_cleanup_ttl(void *arg) {
    ttl_entry_t *ttl_entry = arg;
    if (ttl_entry && ttl_entry->value) {
        free(ttl_entry->value);
    }
}

bool is_entry_expired(ttl_entry_t *entry, time_t current_time) {
    if (entry->ttl == 0) {
        log_debug("Not expired\n");
        return false;
    }
    return difftime(current_time, entry->timestamp) > entry->ttl;
}

void ttl_set(char *key, char *value, char *ttl_value, char *response) {
    char *response_value = NULL;
    // Set a key value on hashmap
    response_value = "OK";
    log_debug("key: %s\n", key);
    log_debug("value: %s\n", value);
    log_debug("ttl_value: %s\n", ttl_value);

    ttl_entry_t new_ttl_entry;
    new_ttl_entry.value = strdup(value);
    if (!new_ttl_entry.value) {
        fprintf(stderr, "failed to allocate memory during hash_table_set");
        response_value = "ERROR: MEMORY ALLOC FAILURE";
        write_response_str(response, response_value);
        return;
    }
    new_ttl_entry.timestamp = time(NULL);

    char *endptr;
    errno = 0;
    long ttl = strtol(ttl_value, &endptr, 10);
    if (errno != 0 || endptr == ttl_value || *endptr != '\0' || ttl < 0) {
        response_value = "ERROR: INVALID TTL";
        write_response_str(response, response_value);
        free(new_ttl_entry.value);
        return;
    }

    new_ttl_entry.ttl = (int)ttl;

    int error =
        hash_table_set(ttl_manager->hash_table_main, key, &new_ttl_entry,
                       sizeof(new_ttl_entry), custom_cleanup_ttl);
    if (error != 0) {
        fprintf(stderr, "failed to allocate memory during set_value");
        response_value = "ERROR: MEMORY ALLOC FAILURE";
    }

    write_response_str(response, response_value);
}

void ttl_get(char *key, char *response) {
    char *response_value = NULL;
    // Get a value from hashmap
    ttl_entry_t *ttl_entry = hash_table_get(ttl_manager->hash_table_main, key);
    if (ttl_entry == NULL) {
        response_value = "ERROR: KEY NOT FOUND";
    } else {
        time_t current_time = time(NULL);
        if (is_entry_expired(ttl_entry, current_time)) {
            // Expired
            hash_table_remove(ttl_manager->hash_table_main, key,
                              custom_cleanup_ttl);
            response_value = "ERROR: KEY NOT FOUND";
        } else {
            // Valid
            response_value = ttl_entry->value;
        }
    }
    write_response_str(response, response_value);
}

void ttl_delete(char *key, char *response) {
    char *response_value = NULL;
    // Delete a value
    response_value = "OK";
    ttl_entry_t *ttl_entry = hash_table_get(ttl_manager->hash_table_main, key);
    if (ttl_entry == NULL) {
        response_value = "ERROR: KEY NOT FOUND";
    } else {
        hash_table_remove(ttl_manager->hash_table_main, key,
                          custom_cleanup_ttl);
    }
    write_response_str(response, response_value);
}
