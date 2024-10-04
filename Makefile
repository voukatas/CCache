CC = gcc -std=gnu11
# Flags to check for race conditions
# You might need this in order to run properly
# sudo sysctl vm.mmap_rnd_bits=28
#CFLAGS = -Iunity -Iinclude -Wall -Wextra -Werror -g -pthread -fsanitize=thread -O0
CFLAGS = -Iunity -Iinclude -Wall -Wextra -Werror -g -pthread -fprofile-arcs -ftest-coverage -O0 
OPTIMIZED_FLAGS = -Iinclude -Wall -Wextra -Werror -O2 -s -pthread -DNDEBUG

APP_SRC = $(wildcard src/*.c)
TEST_SRC = $(wildcard test/*.c) $(wildcard unity/*.c)

APP_OBJ = $(APP_SRC:.c=.o)
TEST_APP_SRC = $(filter-out src/main.c, $(APP_SRC))
TEST_OBJ = $(TEST_SRC:.c=.o) $(patsubst src/%.c,$(BUILD_TARGET)/%.o,$(TEST_APP_SRC))

OPTIMIZED_OBJ = $(APP_SRC:.c=.optimized.o)

APP_TARGET = ccache_dbg
TEST_TARGET = test_app
OPTIMIZED_TARGET = ccache_prod

BUILD_TARGET = test_obj

ifndef EVICTION_POLICY
    EVICTION_POLICY = TTL
endif

CPPFLAGS += -DEVICTION_FLAG_$(EVICTION_POLICY)

all: $(APP_TARGET) $(TEST_TARGET) $(OPTIMIZED_TARGET)
	@echo "CPPFLAGS = $(CPPFLAGS)"

# Create the application binary
$(APP_TARGET): $(APP_OBJ)
	$(CC) $(CFLAGS) $(CPPFLAGS) -o $@ $^

# Create the test binary
$(TEST_TARGET): $(TEST_OBJ)
	$(CC) $(CFLAGS) $(CPPFLAGS) -DTESTING -o $@ $^

# Rule to create the optimized production binary
$(OPTIMIZED_TARGET): $(OPTIMIZED_OBJ)
	$(CC) $(OPTIMIZED_FLAGS) $(CPPFLAGS) -o $@ $^

# General rule
%.o: %.c
	$(CC) $(CFLAGS) $(CPPFLAGS) -c $< -o $@

%.optimized.o: %.c
	$(CC) $(OPTIMIZED_FLAGS) $(CPPFLAGS) -c $< -o $@

# Compile test object files with TESTING defined
$(BUILD_TARGET)/%.o: src/%.c
	mkdir -p $(BUILD_TARGET)
	$(CC) $(CFLAGS) $(CPPFLAGS) -DTESTING -c $< -o $@

clean:
	rm -f $(APP_OBJ) $(TEST_OBJ) $(APP_TARGET) $(TEST_TARGET) $(OPTIMIZED_TARGET) $(OPTIMIZED_OBJ)
	rm -rf $(BUILD_TARGET)/
	rm -rf test/coverage.info test/out/ test_app.c.gcov
	rm test/mock_malloc.gcda test/mock_malloc.gcno test/test_app.gcda test/test_app.gcno unity/unity.gcda unity/unity.gcno
	rm -rf main.c.gcov server.c.gcov src/connection.gcda src/connection.gcno src/coverage.info src/hashtable.gcda src/hashtable.gcno src/main.gcda src/main.gcno src/out/ src/server.gcda src/server.gcno src/signal_handler.gcda src/signal_handler.gcno src/logger.gcno src/logger.gcda src/ttl_manager.gcno src/ttl_manager.gcda test/test_ttl_cache.gcno test/test_ttl_cache.gcda test/test_lru_cache.gcno test/test_lru_cache.gcda test/util.gcno test/util.gcda

