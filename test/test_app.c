#include <stdio.h>
#define TESTING
// #include "../include/hashtable.h"
// #include "../include/mock_malloc.h"
#include <arpa/inet.h>
#include <pthread.h>

#include "../include/connection.h"
#include "../include/server.h"
#include "../include/signal_handler.h"
#include "../unity/unity.h"

// Messages from the tests use the stderr to avoid valgrind detect memory leaks
// in printf/stdout. Not an issue. void server_delay(unsigned int delay);
static void connect_disconnect_client(int port, char *ip);

// vars
static const unsigned int WAITING_SERVER_TIMEOUT = 5 * 1000 * 1000;  // 5 sec
static const unsigned int WAITING_SERVER_INIT = 5 * 1000 * 1000;     // 5 sec
//
static const int TEST_PORT = 8080;
static char *TEST_ADDRESS = "127.0.0.1";

static pthread_t server_thread;

// A small delay for server to start and avoid connection refusals
// void server_delay(unsigned int delay) { usleep(delay); }

// Helper funtions
static void print_msg(char *msg) { fprintf(stderr, "%s", msg); }

static void wait_for_no_active_connections(unsigned int waiting_time) {
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

static void shutdown_server(void) {
    wait_for_no_active_connections(WAITING_SERVER_TIMEOUT);
    // close the event loop
    set_event_loop_state(0);
    print_msg("send disconnect signal\n");
    // this is needed so the event loop get in state of checking again if it is
    // running
    connect_disconnect_client(TEST_PORT, TEST_ADDRESS);
}

static void wait_for_server_to_start(unsigned int waiting_time) {
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

static void start_server(void) {
    wait_for_server_to_start(WAITING_SERVER_INIT);
}

static void *run_server_thread(void *arg) {
    int *port = (int *)arg;
    run_server(*port);
    // run_server(*port, 3, 1);
    return NULL;
}

static int connect_client(int port, char *ip) {
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

static void disconnect_client(int sockfd) { close(sockfd); }

static void connect_disconnect_client(int port, char *ip) {
    int sockfd = connect_client(port, ip);
    // connect_client(port, ip);
    disconnect_client(sockfd);
}

static void send_client_msg_and_wait_response(int sockfd, char *msg,
                                              char *buffer) {
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
    // printf("Msg rcvd: %s", buffer);
    // TEST_ASSERT_EQUAL_STRING("OK\r\n", buffer);

    // disconnect_client(sockfd);
    //  close(sockfd);
}
static void send_client_msg(int sockfd, char *msg) {
    // char buffer[BUFFER_SIZE];

    if (send(sockfd, msg, strlen(msg), 0) < 0) {
        print_msg("Send failed");
        close(sockfd);
        exit(EXIT_FAILURE);
    }
}
static void send_client_msg_in_new_conn_and_wait_response(char *msg, int port,
                                                          char *ip,
                                                          char *buffer) {
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

void setUp(void) {
    static int port = TEST_PORT;
    pthread_create(&server_thread, NULL, run_server_thread, &port);
    start_server();
}

void tearDown(void) {
    shutdown_server();
    pthread_join(server_thread, NULL);
}

// Test cases
void test_run_server_initialization(void) {
    char buffer[BUFFER_SIZE];

    send_client_msg_in_new_conn_and_wait_response(
        "SET test_key test_value 30\r\n", TEST_PORT, TEST_ADDRESS, buffer);
    TEST_ASSERT_EQUAL_STRING("OK\r\n", buffer);
}

void test_run_server_multiple_clients(void) {
    char buffer[BUFFER_SIZE];

    int sockfd1 = connect_client(TEST_PORT, TEST_ADDRESS);
    int sockfd2 = connect_client(TEST_PORT, TEST_ADDRESS);
    int sockfd3 = connect_client(TEST_PORT, TEST_ADDRESS);
    // Verify that the num of clients is 3 using some text-based command
    send_client_msg_and_wait_response(sockfd2, "CONNECTIONS\r\n", buffer);
    TEST_ASSERT_EQUAL_STRING("3\r\n", buffer);

    disconnect_client(sockfd2);
    // Verify that the num of clients is 2 using some text-based command
    send_client_msg_and_wait_response(sockfd3, "CONNECTIONS\r\n", buffer);
    TEST_ASSERT_EQUAL_STRING("2\r\n", buffer);

    disconnect_client(sockfd3);
    disconnect_client(sockfd1);
}

void test_cache_api(void) {
    char buffer[BUFFER_SIZE];

    // Test scenario
    int sockfd = connect_client(TEST_PORT, TEST_ADDRESS);

    // Test SET command
    send_client_msg_and_wait_response(sockfd, "SET test_key test_value 30\r\n",
                                      buffer);
    TEST_ASSERT_EQUAL_STRING("OK\r\n", buffer);
    // re-set the same key
    send_client_msg_and_wait_response(sockfd, "SET test_key test_value 30\r\n",
                                      buffer);
    TEST_ASSERT_EQUAL_STRING("OK\r\n", buffer);

    // Test SET command
    send_client_msg_and_wait_response(
        sockfd, "SET test_key1 test_value1 30\r\n", buffer);
    TEST_ASSERT_EQUAL_STRING("OK\r\n", buffer);

    // Test GET command
    send_client_msg_and_wait_response(sockfd, "GET test_key\r\n", buffer);
    TEST_ASSERT_EQUAL_STRING("test_value\r\n", buffer);

    // Test GET command error
    send_client_msg_and_wait_response(sockfd, "GET test_invalid_key\r\n",
                                      buffer);
    TEST_ASSERT_EQUAL_STRING("ERROR: KEY NOT FOUND\r\n", buffer);

    // Test GET command
    send_client_msg_and_wait_response(sockfd, "GET test_key1\r\n", buffer);
    TEST_ASSERT_EQUAL_STRING("test_value1\r\n", buffer);

    // Test GET command
    send_client_msg_and_wait_response(sockfd, "DELETE test_key1\r\n", buffer);
    TEST_ASSERT_EQUAL_STRING("OK\r\n", buffer);

    // Test GET command error
    send_client_msg_and_wait_response(sockfd, "GET test_key1\r\n", buffer);
    TEST_ASSERT_EQUAL_STRING("ERROR: KEY NOT FOUND\r\n", buffer);

    // Test CONNECTIONS command
    send_client_msg_and_wait_response(sockfd, "CONNECTIONS\r\n", buffer);
    TEST_ASSERT_EQUAL_STRING("1\r\n", buffer);

    // Test KEYS_NUM command
    send_client_msg_and_wait_response(sockfd, "KEYS_NUM\r\n", buffer);
    TEST_ASSERT_EQUAL_STRING("1\r\n", buffer);

    disconnect_client(sockfd);
}
void test_cache_api_error_command_incomplete(void) {
    char buffer[BUFFER_SIZE];

    // Test scenario
    int sockfd = connect_client(TEST_PORT, TEST_ADDRESS);

    // Test SET command
    send_client_msg_and_wait_response(
        sockfd, "SET test_key1 a_very_long_test_valuva 30", buffer);
    TEST_ASSERT_EQUAL_STRING("ERROR: Command incomplete\r\n", buffer);

    disconnect_client(sockfd);
}

void test_cache_api_error_command_too_large(void) {
    char buffer[BUFFER_SIZE];

    int sockfd = connect_client(TEST_PORT, TEST_ADDRESS);

    // Test SET command
    send_client_msg_and_wait_response(
        sockfd,
        "SET test_key1 a_very_long_test_valuea_very_long_test_val 30\r\n",
        buffer);
    TEST_ASSERT_EQUAL_STRING("ERROR: Command too large\r\n", buffer);

    disconnect_client(sockfd);
}

void test_cache_api_partitioned_msg(void) {
    char buffer[BUFFER_SIZE];

    int sockfd = connect_client(TEST_PORT, TEST_ADDRESS);

    // Test SET command
    send_client_msg(sockfd, "SET test_key1 a_very_lon");
    send_client_msg_and_wait_response(sockfd, "g_key 30\r\n", buffer);
    TEST_ASSERT_EQUAL_STRING("OK\r\n", buffer);

    disconnect_client(sockfd);
}
void test_invalid_ttl_value(void) {
    char buffer[BUFFER_SIZE];

    int sockfd = connect_client(TEST_PORT, TEST_ADDRESS);

    send_client_msg_and_wait_response(
        sockfd, "SET test_key11 test_value -1\r\n", buffer);
    TEST_ASSERT_EQUAL_STRING("ERROR: INVALID TTL\r\n", buffer);

    send_client_msg_and_wait_response(
        sockfd, "SET test_key11 test_value aa\r\n", buffer);
    TEST_ASSERT_EQUAL_STRING("ERROR: INVALID TTL\r\n", buffer);

    disconnect_client(sockfd);
}

void test_passive_ttl_cache(void) {
    char buffer[BUFFER_SIZE];

    int sockfd = connect_client(TEST_PORT, TEST_ADDRESS);

    send_client_msg_and_wait_response(sockfd, "SET test_key11 test_value 1\r\n",
                                      buffer);
    TEST_ASSERT_EQUAL_STRING("OK\r\n", buffer);
    send_client_msg_and_wait_response(sockfd, "GET test_key11\r\n", buffer);
    TEST_ASSERT_EQUAL_STRING("test_value\r\n", buffer);
    send_client_msg_and_wait_response(sockfd, "KEYS_NUM\r\n", buffer);
    TEST_ASSERT_EQUAL_STRING("1\r\n", buffer);

    // Should not expire
    send_client_msg_and_wait_response(
        sockfd, "SET test_key_no_expire no_expire 0\r\n", buffer);
    TEST_ASSERT_EQUAL_STRING("OK\r\n", buffer);
    send_client_msg_and_wait_response(sockfd, "GET test_key_no_expire\r\n",
                                      buffer);
    TEST_ASSERT_EQUAL_STRING("no_expire\r\n", buffer);

    // Test GET command
    sleep(2);
    send_client_msg_and_wait_response(sockfd, "GET test_key11\r\n", buffer);
    TEST_ASSERT_EQUAL_STRING("ERROR: KEY NOT FOUND\r\n", buffer);

    send_client_msg_and_wait_response(sockfd, "GET test_key_no_expire\r\n",
                                      buffer);
    TEST_ASSERT_EQUAL_STRING("no_expire\r\n", buffer);

    send_client_msg_and_wait_response(sockfd, "KEYS_NUM\r\n", buffer);
    TEST_ASSERT_EQUAL_STRING("1\r\n", buffer);

    disconnect_client(sockfd);
}

void test_active_ttl_cache(void) {
    char buffer[BUFFER_SIZE];

    int sockfd = connect_client(TEST_PORT, TEST_ADDRESS);

    send_client_msg_and_wait_response(sockfd, "SET test_key11 test_value 1\r\n",
                                      buffer);
    TEST_ASSERT_EQUAL_STRING("OK\r\n", buffer);
    send_client_msg_and_wait_response(sockfd, "GET test_key11\r\n", buffer);
    TEST_ASSERT_EQUAL_STRING("test_value\r\n", buffer);

    // Should not expire
    send_client_msg_and_wait_response(
        sockfd, "SET test_key_no_expire no_expire 0\r\n", buffer);
    TEST_ASSERT_EQUAL_STRING("OK\r\n", buffer);
    send_client_msg_and_wait_response(sockfd, "GET test_key_no_expire\r\n",
                                      buffer);
    TEST_ASSERT_EQUAL_STRING("no_expire\r\n", buffer);

    send_client_msg_and_wait_response(sockfd, "KEYS_NUM\r\n", buffer);
    TEST_ASSERT_EQUAL_STRING("2\r\n", buffer);

    // Test GET command
    sleep(4);
    send_client_msg_and_wait_response(sockfd, "GET test_key_no_expire\r\n",
                                      buffer);
    TEST_ASSERT_EQUAL_STRING("no_expire\r\n", buffer);

    send_client_msg_and_wait_response(sockfd, "KEYS_NUM\r\n", buffer);
    TEST_ASSERT_EQUAL_STRING("1\r\n", buffer);

    disconnect_client(sockfd);
}
// To-Do
void test_handle_client_read_and_write(void) {}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_run_server_initialization);
    RUN_TEST(test_run_server_multiple_clients);
    RUN_TEST(test_cache_api);
    RUN_TEST(test_cache_api_error_command_too_large);
    RUN_TEST(test_cache_api_error_command_incomplete);
    RUN_TEST(test_passive_ttl_cache);
    RUN_TEST(test_active_ttl_cache);
    RUN_TEST(test_invalid_ttl_value);
    RUN_TEST(test_cache_api_partitioned_msg);

    return UNITY_END();
}
