#include "util.h"

// Test variables
// char test_output_buffer[1024];
// Helper functions
// void mock_response_writer(char *response, char *response_value) {
//     (void)response;
//     snprintf(test_output_buffer, sizeof(test_output_buffer), "%s",
//              response_value);
// }

// Test cases

// Unit test
// void test_lru_cache_set(void) {
//     lru_manager = malloc(sizeof(lru_manager_t));
//     if (!lru_manager) {
//         log_error("failed to allocate memory: %s for lru manager",
//                   strerror(errno));
//         exit(EXIT_FAILURE);
//     }
//     lru_manager->hash_table_main =
//     hash_table_create(HASH_TABLE_STARTING_SIZE); lru_manager->head = NULL;
//     lru_manager->tail = NULL;
//     lru_manager->size = 0;
//     lru_manager->capacity = LRU_CAPACITY;
//     set_response_writer(mock_response_writer);
//
//     lru_set("key1", "value1", NULL);
//     TEST_ASSERT_EQUAL_STRING("OK", test_output_buffer);
//
//     set_response_writer(write_response_str);
//     hash_table_cleanup(lru_manager->hash_table_main, custom_cleanup_lru);
//     free(lru_manager);
// }

void test_lru_cache_api(void) {
    char buffer[BUFFER_SIZE];

    // Test scenario
    int sockfd = connect_client(TEST_PORT, TEST_ADDRESS);

    // Test SET command
    send_client_msg_and_wait_response(sockfd, "SET test_key test_value\r\n",
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

void test_lru_cache_eviction(void) {
    char buffer[BUFFER_SIZE];

    int sockfd = connect_client(TEST_PORT, TEST_ADDRESS);

    // Set 5 values, the max capacity for testing
    send_client_msg_and_wait_response(sockfd, "SET test_key1 test_value1\r\n",
                                      buffer);
    TEST_ASSERT_EQUAL_STRING("OK\r\n", buffer);

    send_client_msg_and_wait_response(sockfd, "SET test_key2 test_value2\r\n",
                                      buffer);
    TEST_ASSERT_EQUAL_STRING("OK\r\n", buffer);
    send_client_msg_and_wait_response(sockfd, "SET test_key3 test_value3\r\n",
                                      buffer);
    TEST_ASSERT_EQUAL_STRING("OK\r\n", buffer);
    send_client_msg_and_wait_response(sockfd, "SET test_key4 test_value4\r\n",
                                      buffer);
    TEST_ASSERT_EQUAL_STRING("OK\r\n", buffer);
    send_client_msg_and_wait_response(sockfd, "SET test_key5 test_value5\r\n",
                                      buffer);
    TEST_ASSERT_EQUAL_STRING("OK\r\n", buffer);

    send_client_msg_and_wait_response(sockfd, "GET test_key1\r\n", buffer);
    TEST_ASSERT_EQUAL_STRING("test_value1\r\n", buffer);
    send_client_msg_and_wait_response(sockfd, "GET test_key2\r\n", buffer);
    TEST_ASSERT_EQUAL_STRING("test_value2\r\n", buffer);
    send_client_msg_and_wait_response(sockfd, "GET test_key3\r\n", buffer);
    TEST_ASSERT_EQUAL_STRING("test_value3\r\n", buffer);
    send_client_msg_and_wait_response(sockfd, "GET test_key4\r\n", buffer);
    TEST_ASSERT_EQUAL_STRING("test_value4\r\n", buffer);
    send_client_msg_and_wait_response(sockfd, "GET test_key5\r\n", buffer);
    TEST_ASSERT_EQUAL_STRING("test_value5\r\n", buffer);

    // Change a value of an existing key
    send_client_msg_and_wait_response(sockfd, "SET test_key1 test_value01\r\n",
                                      buffer);
    TEST_ASSERT_EQUAL_STRING("OK\r\n", buffer);
    send_client_msg_and_wait_response(sockfd, "GET test_key1\r\n", buffer);
    TEST_ASSERT_EQUAL_STRING("test_value01\r\n", buffer);

    // Add a new key, the test_key2 key should be at the end of the queue
    // therefore be deleted
    send_client_msg_and_wait_response(sockfd, "SET test_key6 test_value6\r\n",
                                      buffer);
    TEST_ASSERT_EQUAL_STRING("OK\r\n", buffer);
    send_client_msg_and_wait_response(sockfd, "GET test_key6\r\n", buffer);
    TEST_ASSERT_EQUAL_STRING("test_value6\r\n", buffer);

    send_client_msg_and_wait_response(sockfd, "GET test_key2\r\n", buffer);
    TEST_ASSERT_EQUAL_STRING("ERROR: KEY NOT FOUND\r\n", buffer);

    send_client_msg_and_wait_response(sockfd, "DELETE test_key1\r\n", buffer);
    TEST_ASSERT_EQUAL_STRING("OK\r\n", buffer);

    send_client_msg_and_wait_response(sockfd, "GET test_key1\r\n", buffer);
    TEST_ASSERT_EQUAL_STRING("ERROR: KEY NOT FOUND\r\n", buffer);

    send_client_msg_and_wait_response(sockfd, "SET test_key7 test_value1\r\n",
                                      buffer);
    TEST_ASSERT_EQUAL_STRING("OK\r\n", buffer);
    send_client_msg_and_wait_response(sockfd, "GET test_key7\r\n", buffer);
    TEST_ASSERT_EQUAL_STRING("test_value1\r\n", buffer);

    send_client_msg_and_wait_response(sockfd, "GET test_key3\r\n", buffer);
    TEST_ASSERT_EQUAL_STRING("test_value3\r\n", buffer);

    send_client_msg_and_wait_response(sockfd, "GET test_key6\r\n", buffer);
    TEST_ASSERT_EQUAL_STRING("test_value6\r\n", buffer);

    send_client_msg_and_wait_response(sockfd, "KEYS_NUM\r\n", buffer);
    TEST_ASSERT_EQUAL_STRING("5\r\n", buffer);

    disconnect_client(sockfd);
}
