CC = gcc
CFLAGS = -Iunity -Iinclude -Wall -Wextra -Werror -g -pthread -fprofile-arcs -ftest-coverage -O0
OPTIMIZED_FLAGS = -Iinclude -Wall -Wextra -Werror -O2 -s -pthread

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

all: $(APP_TARGET) $(TEST_TARGET) $(OPTIMIZED_TARGET)

# Create the application binary
$(APP_TARGET): $(APP_OBJ)
	$(CC) $(CFLAGS) -o $@ $^

# Create the test binary
$(TEST_TARGET): $(TEST_OBJ)
	$(CC) $(CFLAGS) -DTESTING -o $@ $^

# Rule to create the optimized production binary
$(OPTIMIZED_TARGET): $(OPTIMIZED_OBJ)
	$(CC) $(OPTIMIZED_FLAGS) -o $@ $^

# General rule
%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

%.optimized.o: %.c
	$(CC) $(OPTIMIZED_FLAGS) -c $< -o $@

# Compile test object files with TESTING defined
$(BUILD_TARGET)/%.o: src/%.c
	mkdir -p $(BUILD_TARGET)
	$(CC) $(CFLAGS) -DTESTING -c $< -o $@

clean:
	rm -f $(APP_OBJ) $(TEST_OBJ) $(APP_TARGET) $(TEST_TARGET) $(OPTIMIZED_TARGET) $(OPTIMIZED_OBJ)
	rm -rf $(BUILD_TARGET)/
	rm -rf test/coverage.info test/out/ test_app.c.gcov
	rm test/mock_malloc.gcda test/mock_malloc.gcno test/test_app.gcda test/test_app.gcno unity/unity.gcda unity/unity.gcno
	rm -rf main.c.gcov server.c.gcov src/client.gcda src/client.gcno src/coverage.info src/hashtable.gcda src/hashtable.gcno src/main.gcda src/main.gcno src/out/ src/server.gcda src/server.gcno src/signal_handler.gcda src/signal_handler.gcno

