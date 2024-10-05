#ifndef CONFIG_H
#define CONFIG_H

#include "log_level.h"
#include "eviction_policy.h"

// ======================= Versioning =======================
#define CCACHE_VERSION "0.1.0"

// ======================= Default Cache Configuration =======================
/**
 * Production (Default) Configuration
 * Modify the values below to change the default behavior in production.
 */

/** 
 * Port for the cache server.
 * Default: 8080
 */
#define PORT 8080

/**
 * Maximum number of events epoll will grab at once.
 * Controls the event queue size for epoll.
 * Default: 64 events.
 */
#define MAX_EVENTS 64

/**
 * Log level controls the verbosity of logs.
 * Available levels:
 * - LOG_LEVEL_DEBUG: Most verbose logging
 * - LOG_LEVEL_INFO: Informational messages
 * - LOG_LEVEL_WARN: Warnings that require attention
 * - LOG_LEVEL_ERROR: Error messages
 * - LOG_LEVEL_NONE: No logging
 * Default: LOG_LEVEL_DEBUG.
 */
#define LOG_LEVEL LOG_LEVEL_DEBUG

/**
 * Buffer size in bytes for key/value storage.
 * Default: 64 KB (64 * 1024 bytes).
 * 
 * IMPORTANT: DO NOT CHANGE THIS VALUE in production unless necessary.
 */
#define BUFFER_SIZE (64 * 1024)

/**
 * Buffer size in bytes for TTL value.
 * Default: 15, This will covers millions of seconds
 * 
 */
#define BUFFER_SIZE_TTL 15

// ======================= Eviction Policy Configuration =======================
/**
 * Eviction policy for cache items.
 * Available policies:
 * - TTL (Time-To-Live): Items expire after a certain amount of time.
 * - LRU (Least Recently Used): Discards the least recently used items first.
 * Default: TTL.
 */
#ifdef EVICTION_FLAG_TTL
#define EVICTION EVICTION_TTL
#endif
#ifdef EVICTION_FLAG_LRU
#define EVICTION EVICTION_LRU
#endif
#ifndef EVICTION
#define EVICTION EVICTION_TTL
#endif

/**
 * Cleanup interval in seconds for the TTL eviction policy.
 * This defines how frequently expired items are removed.
 * Default: 15 seconds.
 */
#define CLEAN_UP_TIME 15

/**
 * Initial size of the hash table.
 * - For TTL policy, this is the starting size and will dynamically increase.
 * - For LRU policy, this is the maximum number of items the cache can hold.
 *   If using LRU, a larger starting size like 10,000 is recommended.
 * Default: 1000 entries.
 */
#define HASH_TABLE_STARTING_SIZE 1000

/*
 * The maximum capacity an LRU can store
 * */
#define LRU_CAPACITY 10000

// ======================= Testing Configuration =======================
#ifdef TESTING
    /**
     * Testing configuration overrides.
     * The following values overwrite the production defaults for testing purposes.
     */

    /**
     * Buffer size in bytes for key storage during testing.
     * Testing default: 41 bytes.
     */
    #undef BUFFER_SIZE
    #define BUFFER_SIZE 41
    
    /**
     * Cleanup interval in seconds for testing.
     * Testing default: 3 seconds.
     */
    #undef CLEAN_UP_TIME
    #define CLEAN_UP_TIME 3
    
    /**
     * Initial size of the hash table for testing.
     * Testing default: 1000 entries.
     */
    #undef HASH_TABLE_STARTING_SIZE
    #define HASH_TABLE_STARTING_SIZE 1000

    #undef LOG_LEVEL
    #define LOG_LEVEL LOG_LEVEL_DEBUG

    #undef LRU_CAPACITY
    #define LRU_CAPACITY 5

#endif

#endif // CONFIG_H

