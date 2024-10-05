#include "../include/lru_manager.h"

// #include "../include/config.h"
#include "../include/logger.h"
#include "../include/response.h"

void custom_cleanup_lru(void *arg) {
    lru_entry_t *lru_entry = arg;
    if (lru_entry && lru_entry->value && lru_entry->key) {
        free(lru_entry->value);
        free(lru_entry->key);
    }
}

void add_entry_to_front_of_q(lru_entry_t *entry) {
    if (lru_manager->head == NULL) {
        lru_manager->head = entry;
        lru_manager->tail = entry;
        return;
    }
    entry->next = lru_manager->head;
    lru_manager->head->previous = entry;
    lru_manager->head = entry;
}

// It removes all the references from the Q but doesn't delete/free the entry
void remove_entry_from_q(lru_entry_t *entry) {
    if (entry->previous != NULL) {
        entry->previous->next = entry->next;
    } else {
        // If it doesn't have previous then it is the head
        lru_manager->head = entry->next;
    }

    if (entry->next != NULL) {
        entry->next->previous = entry->previous;
    } else {
        // It means it is the tail
        lru_manager->tail = entry->previous;
    }

    entry->previous = NULL;
    entry->next = NULL;
}

// It removes and deletes the entry completely from the Q
// void delete_entry_from_q(lru_entry_t *entry) {
//     remove_entry_from_q(entry);
//     free(entry);
// }

void move_entry_to_front_of_q(lru_entry_t *entry) {
    if (lru_manager->head == entry) {
        return;
    }
    // doesn't free the entry, only removes the refernces
    remove_entry_from_q(entry);
    add_entry_to_front_of_q(entry);
}

// lru_set
void lru_set(char *key, char *value, char *response) {
    char *response_value = NULL;
    log_debug("key: %s\n", key);
    log_debug("value: %s\n", value);

    lru_entry_t *lru_entry = hash_table_get(lru_manager->hash_table_main, key);
    if (lru_entry != NULL) {
        // move to front of list
        move_entry_to_front_of_q(lru_entry);

        // change the value
        void *new_value_ptr = strdup(value);
        if (!new_value_ptr) {
            fprintf(stderr,
                    "failed to allocate memory for new value during lru_set");
            response_value = "ERROR: MEMORY ALLOC FAILURE";
            write_response_str(response, response_value);
            return;
        }
        // free the old value
        free(lru_entry->value);
        // set the new value to the entry
        lru_entry->value = new_value_ptr;

        // send response
        response_value = "OK";
        write_response_str(response, response_value);
        return;
    }

    // key doesn't exist, so create a new and store it

    lru_entry_t new_lru_entry;
    new_lru_entry.value = strdup(value);
    if (!new_lru_entry.value) {
        fprintf(stderr,
                "failed to allocate memory during setting new entry value in "
                "lru_set");
        response_value = "ERROR: MEMORY ALLOC FAILURE";
        write_response_str(response, response_value);
        return;
    }
    new_lru_entry.key = strdup(key);
    if (!new_lru_entry.key) {
        free(new_lru_entry.value);
        fprintf(stderr,
                "failed to allocate memory during setting new entry key in "
                "lru_set");
        response_value = "ERROR: MEMORY ALLOC FAILURE";
        write_response_str(response, response_value);
        return;
    }
    new_lru_entry.next = NULL;
    new_lru_entry.previous = NULL;

    // check if it fits in the current capacity and if not remove the last entry
    // from the Q
    if (lru_manager->size >= lru_manager->capacity) {
        // remove it from the Q
        remove_entry_from_q(lru_manager->tail);
        // remove the last entry from hashtable
        hash_table_remove(lru_manager->hash_table_main, lru_manager->tail->key,
                          custom_cleanup_lru);
        lru_manager->size--;
    }

    int error =
        hash_table_set(lru_manager->hash_table_main, key, &new_lru_entry,
                       sizeof(new_lru_entry), custom_cleanup_lru);
    if (error != 0) {
        fprintf(stderr, "failed to allocate memory during lru set_value");
        response_value = "ERROR: MEMORY ALLOC FAILURE";
        write_response_str(response, response_value);
        return;
    }

    // This is neccesary because the hashtable recreates it's own entries and we
    // want the actual created entry. Can be optimized by modifing the hashtable
    // to return the value when we set a new entry
    lru_entry_t *lru_entry_new =
        hash_table_get(lru_manager->hash_table_main, key);
    if (lru_entry_new == NULL) {
        fprintf(stderr, "failed, something went terribly wrong! ");
        response_value = "ERROR: UNEXPECTED FAILURE ON RETRIEVING THE KEY";
        write_response_str(response, response_value);
        return;
    }

    add_entry_to_front_of_q(lru_entry_new);
    lru_manager->size++;

    response_value = "OK";
    write_response_str(response, response_value);
}

// lru_get
void lru_get(char *key, char *response) {
    char *response_value = NULL;
    lru_entry_t *lru_entry = hash_table_get(lru_manager->hash_table_main, key);

    if (lru_entry == NULL) {
        response_value = "ERROR: KEY NOT FOUND";
        write_response_str(response, response_value);
        return;
    }

    response_value = lru_entry->value;
    move_entry_to_front_of_q(lru_entry);
    write_response_str(response, response_value);
}

// lru_delete
void lru_delete(char *key, char *response) {
    char *response_value = NULL;
    lru_entry_t *lru_entry = hash_table_get(lru_manager->hash_table_main, key);
    if (lru_entry == NULL) {
        response_value = "ERROR: KEY NOT FOUND";
        write_response_str(response, response_value);
        return;
    }
    remove_entry_from_q(lru_entry);
    hash_table_remove(lru_manager->hash_table_main, key, custom_cleanup_lru);
    lru_manager->size--;
    response_value = "OK";
    write_response_str(response, response_value);
}
