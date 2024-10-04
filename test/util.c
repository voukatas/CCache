#include "util.h"
// vars
const unsigned int WAITING_SERVER_TIMEOUT = 5 * 1000 * 1000;  // 5 sec
const unsigned int WAITING_SERVER_INIT = 5 * 1000 * 1000;     // 5 sec
                                                              //
char *TEST_ADDRESS = "127.0.0.1";

// A small delay for server to start and avoid connection refusals
// void server_delay(unsigned int delay) { usleep(delay); }

// Helper funtions
void print_msg(char *msg) { fprintf(stderr, "%s", msg); }

void wait_for_no_active_connections(unsigned int waiting_time) {
    unsigned int total_time = 0;
    const unsigned int delay = 100000;  // 100ms
                                        // atomic_load(&(var))
    while (get_active_connections() > 0) {
        usleep(delay);
        total_time += delay;
        if (total_time > waiting_time) {
            print_msg(
                "Timeout waiting for server to close all the connections\n");
            exit(EXIT_FAILURE);
        }
    }
}

void shutdown_server(void) {
    wait_for_no_active_connections(WAITING_SERVER_TIMEOUT);
    // close the event loop
    set_event_loop_state(0);
    // print_msg("send disconnect signal\n");
    //  this is needed so the event loop get in state of checking again if it is
    //  running
    connect_disconnect_client(TEST_PORT, TEST_ADDRESS);
}

void wait_for_server_to_start(unsigned int waiting_time) {
    unsigned int total_time = 0;
    const unsigned int delay = 100000;  // 100ms
    while (get_server_state() != SERVER_STATE_RUNNING) {
        usleep(delay);
        total_time += delay;
        if (total_time > waiting_time) {
            print_msg("Timeout waiting for server to start running\n");
            exit(EXIT_FAILURE);
        }
    }
}

void start_server(void) { wait_for_server_to_start(WAITING_SERVER_INIT); }

void *run_server_thread(void *arg) {
    int *port = (int *)arg;
    run_server(*port);
    // run_server(*port, 3, 1);
    return NULL;
}

int connect_client(int port, char *ip) {
    int sockfd;
    struct sockaddr_in server_addr;
    // char buffer[BUFFER_SIZE];

    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
        print_msg("Socket creation failed");
        exit(EXIT_FAILURE);
    }

    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);
    if (inet_pton(AF_INET, ip, &server_addr.sin_addr) <= 0) {
        print_msg("Invalid address");
        close(sockfd);
        exit(EXIT_FAILURE);
    }

    if (connect(sockfd, (struct sockaddr *)&server_addr, sizeof(server_addr)) <
        0) {
        print_msg("Connection to the server failed");
        close(sockfd);
        // exit(EXIT_FAILURE);
    }

    return sockfd;
}

void disconnect_client(int sockfd) { close(sockfd); }

void connect_disconnect_client(int port, char *ip) {
    int sockfd = connect_client(port, ip);
    // connect_client(port, ip);
    disconnect_client(sockfd);
}

void send_client_msg_and_wait_response(int sockfd, char *msg, char *buffer) {
    // char buffer[BUFFER_SIZE];

    if (send(sockfd, msg, strlen(msg), 0) < 0) {
        print_msg("Send failed");
        close(sockfd);
        exit(EXIT_FAILURE);
    }

    int bytes_received = read(sockfd, buffer, BUFFER_SIZE);
    if (bytes_received < 0) {
        print_msg("Receive failed");
        close(sockfd);
        exit(EXIT_FAILURE);
    }
    buffer[bytes_received] = '\0';
}
void send_client_msg(int sockfd, char *msg) {
    // char buffer[BUFFER_SIZE];

    if (send(sockfd, msg, strlen(msg), 0) < 0) {
        print_msg("Send failed");
        close(sockfd);
        exit(EXIT_FAILURE);
    }
}
void send_client_msg_in_new_conn_and_wait_response(char *msg, int port,
                                                   char *ip, char *buffer) {
    // char buffer[BUFFER_SIZE];
    int sockfd = connect_client(port, ip);

    if (send(sockfd, msg, strlen(msg), 0) < 0) {
        print_msg("Send failed");
        close(sockfd);
        exit(EXIT_FAILURE);
    }

    int bytes_received = read(sockfd, buffer, BUFFER_SIZE);
    if (bytes_received < 0) {
        print_msg("Receive failed");
        close(sockfd);
        exit(EXIT_FAILURE);
    }
    buffer[bytes_received] = '\0';
    // printf("Msg rcvd: %s", buffer);
    // TEST_ASSERT_EQUAL_STRING("OK\r\n", buffer);

    disconnect_client(sockfd);
    // close(sockfd);
}
