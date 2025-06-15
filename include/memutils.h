#ifndef MEMUTILS_H
#define MEMUTILS_H
#include <stddef.h>
void *memcpy(void *dst, const void *src, size_t n);
void *memset(void *dst, int val, size_t n);
#endif /* MEMUTILS_H */
