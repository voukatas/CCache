#include "../unity/unity.h"
#include "test.h"
#include "util.h"

pthread_t server_thread;

void setUp(void) {
    static int port = TEST_PORT;
    pthread_create(&server_thread, NULL, run_server_thread, &port);
    start_server();
}

void tearDown(void) {
    shutdown_server();
    pthread_join(server_thread, NULL);
}

int main(void) {
    UNITY_BEGIN();
#ifdef EVICTION_FLAG_TTL
    RUN_TEST(test_run_server_initialization);
    RUN_TEST(test_run_server_multiple_clients);
    RUN_TEST(test_cache_api);
    RUN_TEST(test_cache_api_error_command_too_large);
    RUN_TEST(test_cache_api_error_command_incomplete);
    RUN_TEST(test_passive_ttl_cache);
    RUN_TEST(test_active_ttl_cache);
    RUN_TEST(test_invalid_ttl_value);
    RUN_TEST(test_cache_api_partitioned_msg);
    RUN_TEST(test_cache_api_error_unknown_or_malformed_command);
#endif
#ifdef EVICTION_FLAG_LRU
    RUN_TEST(test_lru_cache_set);
#endif

    return UNITY_END();
}
