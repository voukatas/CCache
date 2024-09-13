#ifndef CONFIG_H
#define CONFIG_H

#define CCACHE_VERSION "0.1.0"

#define PORT 8080
// The number of events that the epoll will grab
#define MAX_EVENTS 64

#ifdef TESTING
//#warning "TESTING macro is defined"
#define BUFFER_SIZE 41
#define CLEAN_UP_TIME 3
#define HASH_TABLE_STARTING_SIZE 1000 
#else
// The maximum bytes a key can hold . 1024*1024
//#define BUFFER_SIZE 1048576
#define BUFFER_SIZE 64*1024
// The interval for the clean up mechanism in seconds
#define CLEAN_UP_TIME 15 
// Starting size of hashtable
#define HASH_TABLE_STARTING_SIZE 1000 
#endif

#endif // CONFIG_H
