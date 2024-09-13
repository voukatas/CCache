#define TESTING
// #include "../include/hashtable.h"
// #include "../include/mock_malloc.h"
#include <arpa/inet.h>
#include <pthread.h>

#include "../include/client.h"
#include "../include/server.h"
#include "../include/signal_handler.h"
#include "../unity/unity.h"

void setUp(void) {}

void tearDown(void) {}

// A small delay for server to start and avoid connection refusals
void server_init_delay() { usleep(100000); }

// Test cases

void test_add_and_clean_client(void) {
    client_node_t *head = NULL;
    node_data_t *client_data = malloc(sizeof(node_data_t));

    int client_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (client_fd == 0) {
        perror("Socket failed");
        exit(EXIT_FAILURE);
    }
    client_t *client = malloc(sizeof(client_t));
    if (!client) {
        perror("malloc failed");
        close(client_fd);
        return;
    }
    // printf("Accepted client socket 1\n");

    client->fd = client_fd;
    client_data->data.client = client;
    client->read_buffer_len = 0;
    client->write_buffer_len = 0;
    client->write_buffer_pos = 0;
    // close(current->client_data->data.client->fd);

    add_client_to_list(&head, client_data);
    TEST_ASSERT_NOT_NULL(head);
    TEST_ASSERT_EQUAL_PTR(head->client_data, client_data);

    cleanup_all_clients(&head);
}

void test_add_and_remove_client(void) {
    client_node_t *head = NULL;
    node_data_t *client_data = malloc(sizeof(node_data_t));

    int client_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (client_fd == 0) {
        perror("Socket failed");
        exit(EXIT_FAILURE);
    }
    client_t *client = malloc(sizeof(client_t));
    if (!client) {
        perror("malloc failed");
        close(client_fd);
        return;
    }
    // printf("Accepted client socket 1\n");

    client->fd = client_fd;
    client_data->data.client = client;
    client->read_buffer_len = 0;
    client->write_buffer_len = 0;
    client->write_buffer_pos = 0;
    // close(current->client_data->data.client->fd);

    add_client_to_list(&head, client_data);
    TEST_ASSERT_NOT_NULL(head);
    TEST_ASSERT_EQUAL_PTR(head->client_data, client_data);

    remove_client_from_list(&head, client_data);
    TEST_ASSERT_NULL(head);
}

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
        perror("Socket creation failed");
        exit(EXIT_FAILURE);
    }

    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);
    if (inet_pton(AF_INET, ip, &server_addr.sin_addr) <= 0) {
        perror("Invalid address");
        close(sockfd);
        exit(EXIT_FAILURE);
    }

    if (connect(sockfd, (struct sockaddr *)&server_addr, sizeof(server_addr)) <
        0) {
        perror("Connection to the server failed");
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
        perror("Send failed");
        close(sockfd);
        exit(EXIT_FAILURE);
    }

    int bytes_received = read(sockfd, buffer, BUFFER_SIZE);
    if (bytes_received < 0) {
        perror("Receive failed");
        close(sockfd);
        exit(EXIT_FAILURE);
    }
    buffer[bytes_received] = '\0';
    // printf("Msg rcvd: %s", buffer);
    // TEST_ASSERT_EQUAL_STRING("OK\r\n", buffer);

    // disconnect_client(sockfd);
    //  close(sockfd);
}
void send_client_msg(int sockfd, char *msg) {
    // char buffer[BUFFER_SIZE];

    if (send(sockfd, msg, strlen(msg), 0) < 0) {
        perror("Send failed");
        close(sockfd);
        exit(EXIT_FAILURE);
    }
}
void send_client_msg_in_new_conn_and_wait_response(char *msg, int port,
                                                   char *ip, char *buffer) {
    // char buffer[BUFFER_SIZE];
    int sockfd = connect_client(port, ip);

    if (send(sockfd, msg, strlen(msg), 0) < 0) {
        perror("Send failed");
        close(sockfd);
        exit(EXIT_FAILURE);
    }

    int bytes_received = read(sockfd, buffer, BUFFER_SIZE);
    if (bytes_received < 0) {
        perror("Receive failed");
        close(sockfd);
        exit(EXIT_FAILURE);
    }
    buffer[bytes_received] = '\0';
    // printf("Msg rcvd: %s", buffer);
    // TEST_ASSERT_EQUAL_STRING("OK\r\n", buffer);

    disconnect_client(sockfd);
    // close(sockfd);
}

void test_run_server_initialization(void) {
    set_event_loop_state(1);
    pthread_t server_thread;
    int port = 8080;

    char buffer[BUFFER_SIZE];

    pthread_create(&server_thread, NULL, run_server_thread, &port);

    // Small delay for server to init
    server_init_delay();
    send_client_msg_in_new_conn_and_wait_response(
        "SET test_key test_value 30\r\n", port, "127.0.0.1", buffer);
    TEST_ASSERT_EQUAL_STRING("OK\r\n", buffer);

    // To shutdown the event loop
    set_event_loop_state(0);
    connect_disconnect_client(port, "127.0.0.1");

    pthread_join(server_thread, NULL);
}

// Placeholder, finish this test after the text-based protocol is finished. This
// covers the branch of code where a node-client from the middle of the linked
// list is removed
void test_run_server_multiple_clients(void) {
    set_event_loop_state(1);
    pthread_t server_thread;
    int port = 8080;
    char *ip = "127.0.0.1";
    char buffer[BUFFER_SIZE];

    pthread_create(&server_thread, NULL, run_server_thread, &port);

    // Small delay for server to init
    server_init_delay();

    // Test scenario
    connect_client(port, ip);
    // send_client_msg("Hello from client\n", port, ip);
    int sockfd2 = connect_client(port, ip);
    int sockfd3 = connect_client(port, ip);
    // Verify that the num of clients is 3 using some text-based command
    send_client_msg_and_wait_response(sockfd2, "CONNECTIONS\r\n", buffer);
    TEST_ASSERT_EQUAL_STRING("3\r\n", buffer);

    disconnect_client(sockfd2);
    // Verify that the num of clients is 2 using some text-based command
    send_client_msg_and_wait_response(sockfd3, "CONNECTIONS\r\n", buffer);
    TEST_ASSERT_EQUAL_STRING("2\r\n", buffer);

    //   To shutdown the event loop
    set_event_loop_state(0);
    connect_disconnect_client(port, "127.0.0.1");

    pthread_join(server_thread, NULL);
}

void test_cache_api(void) {
    set_event_loop_state(1);
    pthread_t server_thread;
    int port = 8080;
    char *ip = "127.0.0.1";
    char buffer[BUFFER_SIZE];

    pthread_create(&server_thread, NULL, run_server_thread, &port);

    // Small delay for server to init
    server_init_delay();

    // Test scenario
    int sockfd = connect_client(port, ip);

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

    //   To shutdown the event loop
    set_event_loop_state(0);
    connect_disconnect_client(port, "127.0.0.1");

    pthread_join(server_thread, NULL);
}
void test_cache_api_error_command_incomplete(void) {
    set_event_loop_state(1);
    pthread_t server_thread;
    int port = 8080;
    char *ip = "127.0.0.1";
    char buffer[BUFFER_SIZE];

    pthread_create(&server_thread, NULL, run_server_thread, &port);

    // Small delay for server to init
    server_init_delay();

    // Test scenario
    int sockfd = connect_client(port, ip);

    // Test SET command
    send_client_msg_and_wait_response(
        sockfd, "SET test_key1 a_very_long_test_valuva 30", buffer);
    TEST_ASSERT_EQUAL_STRING("ERROR: Command incomplete\r\n", buffer);

    disconnect_client(sockfd);

    //   To shutdown the event loop
    set_event_loop_state(0);
    connect_disconnect_client(port, "127.0.0.1");

    pthread_join(server_thread, NULL);
}

void test_cache_api_error_command_too_large(void) {
    set_event_loop_state(1);
    pthread_t server_thread;
    int port = 8080;
    char *ip = "127.0.0.1";
    char buffer[BUFFER_SIZE];

    pthread_create(&server_thread, NULL, run_server_thread, &port);

    // Small delay for server to init
    server_init_delay();

    // Test scenario
    int sockfd = connect_client(port, ip);

    // Test SET command
    send_client_msg_and_wait_response(
        sockfd,
        "SET test_key1 a_very_long_test_valuea_very_long_test_val 30\r\n",
        buffer);
    TEST_ASSERT_EQUAL_STRING("ERROR: Command too large\r\n", buffer);

    disconnect_client(sockfd);

    //   To shutdown the event loop
    set_event_loop_state(0);
    connect_disconnect_client(port, "127.0.0.1");

    pthread_join(server_thread, NULL);
}

void test_cache_api_partitioned_msg(void) {
    set_event_loop_state(1);
    pthread_t server_thread;
    int port = 8080;
    char *ip = "127.0.0.1";
    char buffer[BUFFER_SIZE];

    pthread_create(&server_thread, NULL, run_server_thread, &port);

    // Small delay for server to init
    server_init_delay();

    // Test scenario
    int sockfd = connect_client(port, ip);

    // Test SET command
    send_client_msg(sockfd, "SET test_key1 a_very_lon");
    send_client_msg_and_wait_response(sockfd, "g_key 30\r\n", buffer);
    TEST_ASSERT_EQUAL_STRING("OK\r\n", buffer);

    disconnect_client(sockfd);

    //   To shutdown the event loop
    set_event_loop_state(0);
    connect_disconnect_client(port, "127.0.0.1");

    pthread_join(server_thread, NULL);
}
void test_invalid_ttl_value(void) {
    set_event_loop_state(1);
    pthread_t server_thread;
    int port = 8080;
    char *ip = "127.0.0.1";
    char buffer[BUFFER_SIZE];

    pthread_create(&server_thread, NULL, run_server_thread, &port);

    // Small delay for server to init
    server_init_delay();

    // Test scenario
    int sockfd = connect_client(port, ip);

    send_client_msg_and_wait_response(
        sockfd, "SET test_key11 test_value -1\r\n", buffer);
    TEST_ASSERT_EQUAL_STRING("ERROR: INVALID TTL\r\n", buffer);

    send_client_msg_and_wait_response(
        sockfd, "SET test_key11 test_value aa\r\n", buffer);
    TEST_ASSERT_EQUAL_STRING("ERROR: INVALID TTL\r\n", buffer);

    disconnect_client(sockfd);

    //   To shutdown the event loop
    set_event_loop_state(0);
    connect_disconnect_client(port, "127.0.0.1");

    pthread_join(server_thread, NULL);
}

void test_passive_ttl_cache(void) {
    set_event_loop_state(1);
    pthread_t server_thread;
    int port = 8080;
    char *ip = "127.0.0.1";
    char buffer[BUFFER_SIZE];

    pthread_create(&server_thread, NULL, run_server_thread, &port);

    // Small delay for server to init
    server_init_delay();

    // Test scenario
    int sockfd = connect_client(port, ip);

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

    //   To shutdown the event loop
    set_event_loop_state(0);
    connect_disconnect_client(port, "127.0.0.1");

    pthread_join(server_thread, NULL);
}

void test_active_ttl_cache(void) {
    set_event_loop_state(1);
    pthread_t server_thread;
    int port = 8080;
    char *ip = "127.0.0.1";
    char buffer[BUFFER_SIZE];

    pthread_create(&server_thread, NULL, run_server_thread, &port);

    // Small delay for server to init
    server_init_delay();

    // Test scenario
    int sockfd = connect_client(port, ip);

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
    // send_client_msg(sockfd, "GET test_key11\r\n", buffer);
    // TEST_ASSERT_EQUAL_STRING("ERROR: KEY NOT FOUND\r\n", buffer);

    disconnect_client(sockfd);

    //   To shutdown the event loop
    set_event_loop_state(0);
    connect_disconnect_client(port, "127.0.0.1");

    pthread_join(server_thread, NULL);
}
// To-Do
void test_handle_client_read_and_write(void) {}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_add_and_remove_client);
    RUN_TEST(test_add_and_clean_client);
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
