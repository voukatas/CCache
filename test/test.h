#ifndef TEST_H
#define TEST_H
void test_active_ttl_cache(void);
void test_passive_ttl_cache(void);
void test_invalid_ttl_value(void);
void test_cache_api_partitioned_msg(void);
void test_cache_api_error_unknown_or_malformed_command(void);
void test_cache_api_error_command_too_large(void);
void test_cache_api_error_command_incomplete(void);
void test_cache_api(void);
void test_run_server_multiple_clients(void);
void test_run_server_initialization(void);
void test_lru_cache_set(void);
#endif
