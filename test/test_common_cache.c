#include "util.h"

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
