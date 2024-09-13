#include "../include/mock_malloc.h"

static int malloc_fail = 0;

void set_malloc_fail(int fail) {
        malloc_fail = fail;
}

void *mock_malloc(size_t size) {
        if (malloc_fail) {
                return NULL;
        }
        return malloc(size);
}

void *mock_calloc(size_t nmemb, size_t size) {
        if (malloc_fail) {
                return NULL;
        }
        return calloc(nmemb, size);
}
