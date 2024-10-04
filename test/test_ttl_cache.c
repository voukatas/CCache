#include "util.h"

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

void test_cache_api_error_unknown_or_malformed_command(void) {
    char buffer[BUFFER_SIZE];

    int sockfd = connect_client(TEST_PORT, TEST_ADDRESS);

    send_client_msg_and_wait_response(sockfd, "PET test_key1 a_val 30\r\n",
                                      buffer);
    TEST_ASSERT_EQUAL_STRING("ERROR: UNKNOWN OR MALFORMED COMMAND\r\n", buffer);

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

void test_ttl_cache_set(void) {
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
