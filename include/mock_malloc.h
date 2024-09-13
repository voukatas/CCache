#ifndef MOCK_MALLOC_H
#define MOCK_MALLOC_H

#include <stdlib.h>

void set_malloc_fail(int fail);

void* mock_malloc(size_t size);
void* mock_calloc(size_t nmemb, size_t size);

#ifdef TESTING
#define malloc(size) mock_malloc(size)
#define calloc(nmemb, size) mock_calloc(nmemb, size)
#endif

#endif

