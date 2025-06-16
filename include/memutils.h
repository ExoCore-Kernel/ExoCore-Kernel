#ifndef MEMUTILS_H
#define MEMUTILS_H
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

void *memcpy(void *dst, const void *src, size_t n);
void *memset(void *dst, int val, size_t n);
int strncmp(const char *s1, const char *s2, size_t n);

#ifdef __cplusplus
}
#endif

#endif /* MEMUTILS_H */
