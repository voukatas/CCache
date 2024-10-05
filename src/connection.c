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
int get_active_connections(void) { return CONNECTIONS_GET(active_connections); }

static void connections_cmd(char *response) {
    write_response_int(response, get_active_connections());
}

static void keys_num_ttl_cmd(char *response) {
    write_response_int(response, ttl_manager->hash_table_main->size);
}
static void keys_num_lru_cmd(char *response) {
    write_response_int(response, lru_manager->hash_table_main->size);
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

// Remove a client from the epoll, so it wont track it
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

static void process_command_for_lru(char *command, char *response) {
    char command_type[20] = {0};
    char key[BUFFER_SIZE] = {0};
    char value[BUFFER_SIZE] = {0};

    int num_args = sscanf(command, "%s %s %s", command_type, key, value);

    log_debug("Command arguments num: %ld", num_args);
    log_debug("command_type: %.3s", command_type);

    if (strncmp(command_type, "SET", 3) == 0 && num_args == 3) {
        lru_set(key, value, response);

    } else if (strncmp(command_type, "GET", 3) == 0 && num_args == 2) {
        lru_get(key, response);

    } else if (strncmp(command_type, "DELETE", 6) == 0 && num_args == 2) {
        lru_delete(key, response);

    } else if (strncmp(command_type, "CONNECTIONS", 11) == 0 && num_args == 1) {
        connections_cmd(response);

    } else if (strncmp(command_type, "KEYS_NUM", 8) == 0 && num_args == 1) {
        keys_num_lru_cmd(response);

    } else {
        // Unknown command
        char *response_value = "ERROR: UNKNOWN OR MALFORMED COMMAND";
        write_response_str(response, response_value);
    }
}

static void process_command_for_ttl(char *command, char *response) {
    char command_type[20] = {0};
    char key[BUFFER_SIZE] = {0};
    char value[BUFFER_SIZE] = {0};
    char ttl_value[BUFFER_SIZE_TTL] = {0};

    int num_args =
        sscanf(command, "%s %s %s %s", command_type, key, value, ttl_value);

    log_debug("Command arguments num: %ld", num_args);
    log_debug("command_type: %.3s", command_type);

    if (strncmp(command_type, "SET", 3) == 0 && num_args == 4) {
        ttl_set(key, value, ttl_value, response);

    } else if (strncmp(command_type, "GET", 3) == 0 && num_args == 2) {
        ttl_get(key, response);

    } else if (strncmp(command_type, "DELETE", 6) == 0 && num_args == 2) {
        ttl_delete(key, response);

    } else if (strncmp(command_type, "CONNECTIONS", 11) == 0 && num_args == 1) {
        connections_cmd(response);

    } else if (strncmp(command_type, "KEYS_NUM", 8) == 0 && num_args == 1) {
        keys_num_ttl_cmd(response);

    } else {
        // Unknown command
        char *response_value = "ERROR: UNKNOWN OR MALFORMED COMMAND";
        write_response_str(response, response_value);
    }
}

void write_response_str(char *response, char *response_value) {
    // Consider the \r\n\0
    if (strlen(response_value) >= BUFFER_SIZE - 3) {
        response_value = "ERROR: RESPONSE OVERFLOW";
    }
    snprintf(response, BUFFER_SIZE, "%s\r\n", response_value);
    log_debug("Processing command response: %s\n", response);
}

void write_response_int(char *response, int response_value) {
    snprintf(response, BUFFER_SIZE, "%d\r\n", response_value);
    log_debug("Processing command response: %s\n", response);
}

static void process_command(char *command, char *response) {
    log_debug("Processing command: %s\n", command);

    if (EVICTION == EVICTION_TTL) {
        log_debug("In EVICTION_TTL mode\n");
        process_command_for_ttl(command, response);

    } else if (EVICTION == EVICTION_LRU) {
        log_debug("In EVICTION_LRU mode\n");
        process_command_for_lru(command, response);
    } else {
        log_error("UNKNOWN EVICTION POLICY mode\n");
    }
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
