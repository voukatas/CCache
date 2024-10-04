#include "../include/server.h"

#include <arpa/inet.h>
#include <assert.h>
#include <bits/time.h>
#include <fcntl.h>
#include <stdio.h>
#include <sys/epoll.h>
#include <sys/timerfd.h>
#include <time.h>

#include "../include/config.h"
#include "../include/connection.h"
#include "../include/signal_handler.h"

CONNECTIONS_TYPE active_connections = CONNECTIONS_INIT;
atomic_int server_state = SERVER_STATE_INITIALIZING;

node_data_t *server_event = NULL;
node_data_t *timer_event = NULL;
hash_table_t *hash_table_main = NULL;
int timer_fd = 0;

void hash_table_cleanup_expired(hash_table_t *ht);

void set_server_state(server_state_t state) {
    atomic_store(&server_state, state);
}
server_state_t get_server_state(void) { return atomic_load(&server_state); }

static void set_non_blocking(int socket) {
    int flags = fcntl(socket, F_GETFL, 0);
    fcntl(socket, F_SETFL, flags | O_NONBLOCK);
}

static int setup_server_socket(int port) {
    int server_fd;  //, client_socket;
    struct sockaddr_in address = {0};

    // Create a socket
    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd == 0) {
        perror("Socket failed");
        exit(EXIT_FAILURE);
    }

    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(port);

    // Set the reuse flag
    int reuse = 1;
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, (char *)&reuse,
                   sizeof(int)) == -1) {
        perror("Set reuse failed");
        close(server_fd);
        exit(EXIT_FAILURE);
    }
    // Bind
    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
        perror("Bind failed");
        close(server_fd);
        exit(EXIT_FAILURE);
    }

    // Listen
    if (listen(server_fd, 3) < 0) {
        perror("Listen failed");
        close(server_fd);
        exit(EXIT_FAILURE);
    }

    set_non_blocking(server_fd);

    return server_fd;
}

// A function that sets a timer using a file descriptor to trigger the cleanupof
// the hashtable
static int setup_cleanup_timerfd() {
    int tfd = timerfd_create(CLOCK_MONOTONIC, 0);
    if (tfd == -1) {
        perror("timerfd_create failed");
        return -1;
    }
    struct itimerspec ts;
    ts.it_value.tv_sec = CLEAN_UP_TIME;  // First trigger
    ts.it_value.tv_nsec = 0;
    ts.it_interval.tv_sec = CLEAN_UP_TIME;  // Interval
    ts.it_interval.tv_nsec = 0;

    if (timerfd_settime(tfd, 0, &ts, NULL) == -1) {
        perror("timerfd_settime failed");
        close(tfd);
        return -1;
    }

    log_info("CLEAN_UP_TIME: %d\n", CLEAN_UP_TIME);

    return tfd;
}

static int setup_epoll(int server_fd, int tfd) {
    int epoll_fd = epoll_create1(0);  // a kernel obj that keeps track of
                                      // multiple fds and notifies on events
    if (epoll_fd == -1) {
        perror("Epoll create failed");
        close(server_fd);
        exit(EXIT_FAILURE);
    }

    server_event = malloc(sizeof(node_data_t));
    if (!server_event) {
        perror("malloc failed");
        close(server_fd);
        exit(EXIT_FAILURE);
    }
    server_event->event_type = EVENT_SERVER;
    server_event->data.fd = server_fd;

    struct epoll_event ev;
    ev.events = EPOLLIN;  // set flag to listen for read events on server socket
    ev.data.ptr = server_event;
    if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, server_fd, &ev) == -1) {
        perror("Epoll ctl add failed");
        close(server_fd);
        close(epoll_fd);
        close(timer_fd);
        free(server_event);
        server_event = NULL;
        exit(EXIT_FAILURE);
    }

    // Add timer_fd to epoll
    struct epoll_event tev;
    tev.events = EPOLLIN;
    timer_event = malloc(sizeof(node_data_t));
    if (!timer_event) {
        perror("malloc failed");
        close(server_fd);
        close(timer_fd);
        close(epoll_fd);
        free(server_event);
        server_event = NULL;
        timer_event = NULL;
        exit(EXIT_FAILURE);
    }
    timer_event->event_type = EVENT_TIMER;
    timer_event->data.fd = tfd;
    // The data.fd and data.ptr is union....
    // tev.data.fd = tfd;
    tev.data.ptr = timer_event;
    if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, tfd, &tev) == -1) {
        perror("Epoll ctl add failed");
        close(server_fd);
        close(epoll_fd);
        free(server_event);
        server_event = NULL;
        exit(EXIT_FAILURE);
    }

    return epoll_fd;
}

static void handle_event(int epoll_fd, struct epoll_event *event) {
    struct sockaddr_in address;
    int addrlen = sizeof(address);
    node_data_t *event_data = (node_data_t *)event->data.ptr;

    if (event_data != NULL && event_data->event_type == EVENT_SERVER) {
        // Accept a new client connection
        int client_socket =
            accept(event_data->data.fd, (struct sockaddr *)&address,
                   (socklen_t *)&addrlen);
        if (client_socket < 0) {
            perror("Accept failed");
            return;
        }

        // Connection accepted at this point

        set_non_blocking(client_socket);

        client_t *client = calloc(1, sizeof(client_t));
        if (!client) {
            perror("malloc failed");
            close(client_socket);
            return;
        }

        client->fd = client_socket;
        client->read_buffer_len = 0;
        client->write_buffer_len = 0;
        client->write_buffer_pos = 0;
        client->close_after_write = 0;

        node_data_t *client_event = malloc(sizeof(node_data_t));
        if (!client_event) {
            perror("malloc failed");
            close(client_socket);
            free(client);
            client = NULL;
            return;
        }
        client_event->event_type = EVENT_CLIENT;
        client_event->data.client = client;

        // Add the client socket to epoll
        struct epoll_event ev;
        ev.events =
            EPOLLIN | EPOLLET | EPOLLET | EPOLLHUP | EPOLLERR | EPOLLRDHUP;
        ev.data.ptr = client_event;
        if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, client_socket, &ev) == -1) {
            perror("Epoll ctl add failed");
            close(client_socket);
            free(client);
            client = NULL;
            free(client_event);
            client_event = NULL;
            return;
        }

        increment_active_connections();
        log_debug("Added Client: %p\n", client);
    } else if (event_data != NULL && event_data->event_type == EVENT_CLIENT) {
        client_t *client = event_data->data.client;

        //  Handle client disconnect or error
        if (event->events & (EPOLLHUP | EPOLLERR | EPOLLRDHUP)) {
            log_debug("Disconnect event for %p\n", event_data->data.client);
            delete_resources(epoll_fd, client, event);
            return;
        }

        // Handle the client request
        if (event->events & EPOLLIN) {
            handle_client_read(client, event, epoll_fd);
        }
        // use an if-else clause here if you want more resource fairness and not
        // speed on response
        if (event->events & EPOLLOUT) {
            handle_client_write(client, event, epoll_fd);
        }

    } else if (event_data != NULL && event_data->event_type == EVENT_TIMER) {
        uint64_t expirations;
        ssize_t bytes = read(timer_fd, &expirations,
                             sizeof(expirations));  // Acknowledge the timer
        if (bytes == -1) {
            perror("read from timer_fd failed");
            return;
        }
        hash_table_cleanup_expired(hash_table_main);
    }
}

void hash_table_cleanup_expired(hash_table_t *ht) {
    log_info("CleanUp Triggered\n");

    time_t current_time = time(NULL);

    for (int i = 0; i < ht->capacity; i++) {
        hash_entry_t *entry = ht->table[i];
        while (entry != NULL) {
            ttl_entry_t *ttl_entry_node = entry->value;
            hash_entry_t *temp = entry->next;
            if (is_entry_expired(ttl_entry_node, current_time)) {
                log_debug("CleanUp Key: %s\n", entry->key);
                hash_table_remove(hash_table_main, entry->key, custom_cleanup);
            }
            entry = temp;
        }
    }

    log_info("CleanUp Ended\n");
}

int run_server(int port) {
    // #warning "TESTING macro is defined"
    CONNECTIONS_STORE(active_connections, CONNECTIONS_INIT);
    KEEP_RUNNING_STORE(keep_running, KEEP_RUNNING_INIT);
    // keep_running = 1;
    set_server_state(SERVER_STATE_INITIALIZING);
    // Create hashtable
    hash_table_main = hash_table_create(HASH_TABLE_STARTING_SIZE);
    //  Set up signal handling
    setup_signal_handling();
    int server_fd = setup_server_socket(port);
    timer_fd = setup_cleanup_timerfd();
    int epoll_fd = setup_epoll(server_fd, timer_fd);

    log_debug("SERVER STARTED\n");
    log_info("Server is listening on port %d\n", port);

    set_server_state(SERVER_STATE_RUNNING);

    struct epoll_event events[MAX_EVENTS];

    // Event Loop
    while (KEEP_RUNNING_LOAD(keep_running)) {
        int nfds =
            epoll_wait(epoll_fd, events, MAX_EVENTS,
                       -1);  // returns as soon as an event occurs, no delay
        if (!KEEP_RUNNING_LOAD(keep_running)) {
            break;
        }
        if (nfds == -1) {
            if (errno == EINTR) {
                break;
            } else {
                perror("epoll_wait failed");
                break;
            }
        }

        for (int i = 0; i < nfds; ++i) {
            handle_event(epoll_fd, &events[i]);
        }
    }
    // Cleanup on shutdown
    // cleanup_all_clients(&client_list_head);
    set_server_state(SERVER_STATE_STOPPED);
    free(server_event);
    server_event = NULL;
    free(timer_event);
    timer_event = NULL;
    close(epoll_fd);
    close(server_fd);
    close(timer_fd);
    hash_table_cleanup(hash_table_main, custom_cleanup);
    CONNECTIONS_STORE(active_connections, CONNECTIONS_INIT);
    // active_connections = CONNECTIONS_INIT;
    log_info("SERVER STOPPED\n");
    return 0;
}
