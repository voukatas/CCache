#include "util.h"

// Test cases

void test_lru_cache_set(void) {}

void test_run_server_multiple_clients_lru_cache(void) {
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
void test_lru_cache_api(void) {
    char buffer[BUFFER_SIZE];

    // Test scenario
    int sockfd = connect_client(TEST_PORT, TEST_ADDRESS);

    // Test SET command
    send_client_msg_and_wait_response(sockfd, "SET test_key test_value \r\n",
                                      buffer);
    TEST_ASSERT_EQUAL_STRING("OK\r\n", buffer);
    // re - set the same key
    send_client_msg_and_wait_response(sockfd, "SET test_key test_value\r\n",
                                      buffer);
    TEST_ASSERT_EQUAL_STRING("OK\r\n", buffer);

    // Test SET command
    send_client_msg_and_wait_response(sockfd, "SET test_key1 test_value1\r\n",
                                      buffer);
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
