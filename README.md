# CCache

A single-threaded event-driven cache

## Features
- Event-driven, single-threaded
- Uses a text-based protocol
- For resource management there are two options, either build with a Time-To-Live (TTL) or Least Recently Used (LRU)

### Project Origin

This project is a continuation and expansion of a smaller side project originally developed in a [different repository](https://github.com/voukatas/C-Playground). The initial version served as a prototype and was part of a collection of various smaller projects. For the original base code and the early development history, please visit the [original repository](https://github.com/voukatas/C-Playground).


## Configuration

The cache is configured using the `config.h` file. The following are the current default configurations:

- **Maximum Value Size**: Configured to support almost 64K bytes as a value. The actual size is slightly reduced to accommodate the `SET` keyword and the bytes for the key.
- **Port**: The server listens on port `8080`.
- **Maximum Epoll Events**: The maximum number of events that `epoll` can retrieve is set to `64`.
- **Cleanup Interval**: The cleanup mechanism for expired keys kicks in every `60` seconds.
- **Initial Hashtable Size**: The starting size of the internal core hashtable is set to `1000`.

## How to run
```bash
# For building with an LRU cache
make EVICTION_POLICY=LRU
# or this for building with an TTL cache
make EVICTION_POLICY=TTL
# For production execute this
./ccache_prod
# For debugging With debug symbols
./ccache_dbg
# To check for memory leaks
valgrind --leak-check=full --track-origins=yes -s ./ccache_dbg
```

## How to run tests
```bash
# check for memory issues
valgrind --leak-check=full --track-origins=yes -s ./test_app

# All in one
# For LRU
make clean && make EVICTION_POLICY=LRU && valgrind --leak-check=full --track-origins=yes -s ./test_app && valgrind --tool=helgrind ./test_app
# or for TTL
make clean && make EVICTION_POLICY=TTL && valgrind --leak-check=full --track-origins=yes -s ./test_app && valgrind --tool=helgrind ./test_app

```
## Check the Code Coverage
```bash
#Enable code coverage flags in Makefile
make test_app
./test_app
# check quickly the coverage
#gcov your_file (eg. src/server.c)
cd test_obj/
lcov --capture --directory . --output-file coverage.info && genhtml coverage.info --output-directory out
# open out/index.html

# Cleanup
make clean

```
## Manual check
```bash
# 30 seconds TTL
echo -ne "SET key_test value_test 30\r\n" | nc localhost 8080
```

## API Usage
CCache is a text-based protocol key-value store that supports basic operations like SET, GET, DELETE, and a few utility commands. Below is a description of the commands supported by the cache and how to use them:

**NOTE: USE THE <TTL> ONLY IF YOU SET IT UP IN TTL MODE, FOR LRU MODE OMIT THIS FIELD**

1. SET Command
- Usage: SET <key> <value> <TTL>
- Description: Sets a key-value pair in the cache with a Time-To-Live (TTL) in seconds. If TTL is set to 0, then the key doesn't expire.
- Response: OK\r\n if the operation is successful. Returns an error if memory allocation fails or the TTL is invalid.
Example:
```bash
SET my_key my_value 30\r\n
OK\r\n
```
2. GET Command
- Usage: GET <key>
- Description: Retrieves the value for a given key if it exists and has not expired.
- Response: <value>\r\n if the key exists and is valid. Returns ERROR: KEY NOT FOUND\r\n if the key does not exist or has expired.
Example:
```bash
GET my_key\r\n
my_value\r\n
```
3. DELETE Command
- Usage: DELETE <key>
- Description: Removes a key-value pair from the cache if it exists.
- Response: OK\r\n if the key was successfully deleted. Returns ERROR: KEY NOT FOUND\r\n if the key does not exist.
Example:
```bash
DELETE my_key\r\n
OK\r\n
```
4. CONNECTIONS Command
- Usage: CONNECTIONS
- Description: Returns the number of active client connections to the cache server.
- Response: <number>\r\n, where <number> is the count of active connections.
Example:
```bash
CONNECTIONS\r\n
1\r\n
```
5. KEYS_NUM Command
- Usage: KEYS_NUM
- Description: Returns the number of keys currently stored in the cache.
- Response: <number>\r\n, where <number> is the count of keys.
Example:
```bash
KEYS_NUM\r\n
2\r\n
```
### Error Handling
- Malformed Commands: Returns ERROR: UNKNOWN OR MALFORMED COMMAND\r\n if a command is not recognized or is improperly formatted.
- Memory Allocation Failures: Returns ERROR: MEMORY ALLOC FAILURE\r\n if there is an issue allocating memory for the operation.
- TTL Issues: Returns ERROR: INVALID TTL\r\n if the TTL provided for a SET command is not a positive integer.

### Example Usage
Below is an example of how to interact with the cache using netcat (nc):
```bash
# Set a key-value pair with a TTL of 30 seconds
echo -ne "SET test_key test_value 30\r\n" | nc localhost 8080

# Retrieve the value for an existing key
echo -ne "GET test_key\r\n" | nc localhost 8080

# Attempt to retrieve a non-existent key
echo -ne "GET invalid_key\r\n" | nc localhost 8080

# Delete an existing key
echo -ne "DELETE test_key\r\n" | nc localhost 8080

# Get the number of active connections
echo -ne "CONNECTIONS\r\n" | nc localhost 8080

# Get the number of keys currently in the cache
echo -ne "KEYS_NUM\r\n" | nc localhost 8080
```
## Dependencies
CCache uses a custom hashtable library to manage key-value pairs efficiently. This library is developed as a separate project and can be found on GitHub on the same repo.

## To-Do
- ~~Either remove the linked list that keeps track of the connections (since it adds management complexity) or implement a periodic validation of the connections~~
- Add API command to change the logging on the fly
- Add support for more cache eviction policies (e.g., LRU, LFU)
- Add more unit tests to increase code coverage and stability
- Replace the stack allocations with heap allocations for the arrays that use the BUFFER_SIZE to support more than 64Kb key-value values. (Currently if more bytes are used it will overflow the stack)
