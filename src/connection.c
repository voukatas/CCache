#include "../include/connection.h"

#include <stdio.h>
#include <string.h>
#include <time.h>

void increment_active_connections(void) {
    CONNECTIONS_INCREMENT(active_connections);
}
void decrement_active_connections(void) {
    CONNECTIONS_DECREMENT(active_connections);
}
int get_active_connections(void) {
    // #warning "TESTING macro is defined"
    //  perror("-----get_active_connections");
    return CONNECTIONS_GET(active_connections);
}

// A custom clean function that is used as a callback in the hashtable
void custom_cleanup(void *arg) {
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

// Remove client from the linked list
void remove_client_from_list(node_data_t *client_data) {
    if (client_data && client_data->data.client) {
        close(client_data->data.client->fd);
        free(client_data->data.client);
        client_data->data.client = NULL;
    }
    if (client_data) {
        free(client_data);
        client_data = NULL;
    }
    decrement_active_connections();
    return;
}

// Remove a client from the epoll, so it wont track it and remove the client
// from the client linked list
void delete_resources(int epoll_fd, client_t *client, struct epoll_event *ev) {
    if (epoll_ctl(epoll_fd, EPOLL_CTL_DEL, client->fd, NULL) == -1) {
        perror("epoll_ctl: EPOLL_CTL_DEL failed");
    }

    remove_client_from_list((node_data_t *)ev->data.ptr);
}

static void set_error_msg(int epoll_fd, client_t *client,
                          struct epoll_event *ev, const char *error_response) {
    int response_len = strlen(error_response);

    if (client->write_buffer_len + response_len < BUFFER_SIZE) {
        memcpy(client->write_buffer + client->write_buffer_len, error_response,
               response_len);
        client->write_buffer_len += response_len;

        client->close_after_write = 1;

        ev->events = EPOLLOUT | EPOLLET | EPOLLHUP | EPOLLERR | EPOLLRDHUP;
        if (epoll_ctl(epoll_fd, EPOLL_CTL_MOD, client->fd, ev) == -1) {
            perror("Epoll ctl mod failed");
            delete_resources(epoll_fd, client, ev);
        }
    } else {
        fprintf(stderr, "Write buffer overflow\n");
        delete_resources(epoll_fd, client, ev);
    }
}

static void process_command(char *command, char *response) {
    log_debug("Processing command: %s\n", command);

    char *response_value = NULL;
    char command_type[20] = {0};
    char key[BUFFER_SIZE] = {0};
    char value[BUFFER_SIZE] = {0};
    char ttl_value[BUFFER_SIZE] = {0};
    // value[0] = '\0';

    int num_args =
        sscanf(command, "%s %s %s %s", command_type, key, value, ttl_value);

    if (strncmp(command_type, "SET", 3) == 0 && num_args == 4) {
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
            snprintf(response, BUFFER_SIZE, "%s\r\n", response_value);
            log_debug("Processing command response: %s\n", response);
            return;
        }
        new_ttl_entry.timestamp = time(NULL);

        char *endptr;
        errno = 0;
        long ttl = strtol(ttl_value, &endptr, 10);
        if (errno != 0 || endptr == ttl_value || *endptr != '\0' || ttl < 0) {
            response_value = "ERROR: INVALID TTL";
            snprintf(response, BUFFER_SIZE, "%s\r\n", response_value);
            log_debug("Processing command response: %s\n", response);
            free(new_ttl_entry.value);
            return;
        }

        new_ttl_entry.ttl = (int)ttl;

        int error = hash_table_set(hash_table_main, key, &new_ttl_entry,
                                   sizeof(new_ttl_entry), custom_cleanup);
        if (error != 0) {
            fprintf(stderr, "failed to allocate memory during set_value");
            response_value = "ERROR: MEMORY ALLOC FAILURE";
        }

    } else if (strncmp(command_type, "GET", 3) == 0 && num_args == 2) {
        // Get a value from hashmap
        ttl_entry_t *ttl_entry = hash_table_get(hash_table_main, key);
        if (ttl_entry == NULL) {
            response_value = "ERROR: KEY NOT FOUND";
        } else {
            time_t current_time = time(NULL);
            if (is_entry_expired(ttl_entry, current_time)) {
                // Expired
                hash_table_remove(hash_table_main, key, custom_cleanup);
                response_value = "ERROR: KEY NOT FOUND";
            } else {
                // Valid
                response_value = ttl_entry->value;
            }
        }

    } else if (strncmp(command_type, "DELETE", 6) == 0 && num_args == 2) {
        // Delete a value
        response_value = "OK";
        ttl_entry_t *ttl_entry = hash_table_get(hash_table_main, key);
        if (ttl_entry == NULL) {
            response_value = "ERROR: KEY NOT FOUND";
        } else {
            hash_table_remove(hash_table_main, key, custom_cleanup);
        }

    } else if (strncmp(command_type, "CONNECTIONS", 11) == 0 && num_args == 1) {
        // I need to rethink this thing
        snprintf(response, BUFFER_SIZE, "%d\r\n", get_active_connections());
        log_debug("Processing command response: %s\n", response);
        return;
    } else if (strncmp(command_type, "KEYS_NUM", 8) == 0 && num_args == 1) {
        snprintf(response, BUFFER_SIZE, "%d\r\n", hash_table_main->size);
        log_debug("Processing command response: %s\n", response);
        return;
    } else {
        // Unknown command
        response_value = "ERROR: UNKNOWN OR MALFORMED COMMAND";
    }

    // Consider the \r\n\0
    if (strlen(response_value) >= BUFFER_SIZE - 3) {
        response_value = "ERROR: RESPONSE OVERFLOW";
    }
    snprintf(response, BUFFER_SIZE, "%s\r\n", response_value);
    log_debug("Processing command response: %s\n", response);
}

void handle_client_read(client_t *client, struct epoll_event *ev,
                        int epoll_fd) {
    char temp_buffer[BUFFER_SIZE];
    int bytes_read = read(client->fd, temp_buffer, sizeof(temp_buffer));

    log_debug("Received message: %.*s\n", bytes_read, temp_buffer);

    if (bytes_read > 0) {
        // Accumulate read data
        if (client->read_buffer_len + bytes_read < BUFFER_SIZE) {
            memcpy(client->read_buffer + client->read_buffer_len, temp_buffer,
                   bytes_read);
            client->read_buffer_len += bytes_read;

            // Ensure the string is terminated
            if (client->read_buffer_len < BUFFER_SIZE) {
                client->read_buffer[client->read_buffer_len] = '\0';
            }

            // Check if the buffer contains at least one \r\n
            char *first_rn = strstr(client->read_buffer, "\r\n");

            // If a \r\n exists, ensure there's only one and process the
            // command
            if (first_rn != NULL) {
                // Check if there is any data after the first \r\n
                if (first_rn + 2 <
                    client->read_buffer + client->read_buffer_len) {
                    // There's more data after the first \r\n, this as an
                    // error
                    const char *error_response =
                        "ERROR: Multiple commands in one request\r\n";
                    set_error_msg(epoll_fd, client, ev, error_response);
                    return;
                }

                // Happy path
                int command_length = first_rn - client->read_buffer;
                char command[BUFFER_SIZE] = {0};
                strncpy(command, client->read_buffer, command_length);
                command[command_length] = '\0';

                char response[BUFFER_SIZE];
                process_command(command, response);
                int response_len = strlen(response);

                // Check if the write buffer has enough space for the
                // response
                if (client->write_buffer_len + response_len < BUFFER_SIZE) {
                    memcpy(client->write_buffer + client->write_buffer_len,
                           response, response_len);
                    client->write_buffer_len += response_len;
                } else {
                    fprintf(stderr, "Write buffer overflow\n");
                    delete_resources(epoll_fd, client, ev);
                    return;
                }

                client->read_buffer_len = 0;

                // Change the event to monitor for writing
                ev->events =
                    EPOLLOUT | EPOLLET | EPOLLHUP | EPOLLERR | EPOLLRDHUP;
                if (epoll_ctl(epoll_fd, EPOLL_CTL_MOD, client->fd, ev) == -1) {
                    perror("Epoll ctl mod failed");
                    delete_resources(epoll_fd, client, ev);
                    return;
                }

            } else if (client->read_buffer_len == BUFFER_SIZE - 1) {
                // Buffer is full but no complete command, this is an error
                const char *error_response = "ERROR: Command incomplete\r\n";
                set_error_msg(epoll_fd, client, ev, error_response);
                return;
            }
        } else {
            // Buffer overflow, the command is too large
            const char *error_response = "ERROR: Command too large\r\n";
            set_error_msg(epoll_fd, client, ev, error_response);
            return;
        }

    } else if (bytes_read == 0) {
        // Handle disconnect
        log_debug("Client disconnected: %p\n", client);
        delete_resources(epoll_fd, client, ev);
    } else if (bytes_read < 0 && errno != EAGAIN) {
        // Handle read error
        perror("Read error");
        delete_resources(epoll_fd, client, ev);
    }
}

void handle_client_write(client_t *client, struct epoll_event *ev,
                         int epoll_fd) {
    while (client->write_buffer_pos < client->write_buffer_len) {
        int bytes_written =
            write(client->fd, client->write_buffer + client->write_buffer_pos,
                  client->write_buffer_len - client->write_buffer_pos);
        if (bytes_written < 0) {
            if (errno == EAGAIN) {
                // Socket not ready for writing, retry later

                ev->events |= EPOLLOUT;
                if (epoll_ctl(epoll_fd, EPOLL_CTL_MOD, client->fd, ev) == -1) {
                    perror("epoll_ctl: EPOLL_CTL_MOD failed");
                    delete_resources(epoll_fd, client, ev);
                }
                return;
            } else {
                perror("Write error");
                delete_resources(epoll_fd, client, ev);
                return;
            }
        }
        client->write_buffer_pos += bytes_written;
    }

    // Stop monitoring for writable events
    if (client->write_buffer_pos == client->write_buffer_len) {
        client->write_buffer_pos = 0;

        client->write_buffer_len = 0;
        ev->events = EPOLLIN | EPOLLET | EPOLLHUP | EPOLLERR | EPOLLRDHUP;

        if (client->close_after_write) {
            delete_resources(epoll_fd, client, ev);
            return;
        }

        if (epoll_ctl(epoll_fd, EPOLL_CTL_MOD, client->fd, ev) == -1) {
            perror("Epoll ctl mod failed");
            delete_resources(epoll_fd, client, ev);
            return;
        }
    }
}
