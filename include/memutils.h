#ifndef MEMUTILS_H
#define MEMUTILS_H
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

void *memcpy(void *dst, const void *src, size_t n);
void *memset(void *dst, int val, size_t n);
int strncmp(const char *s1, const char *s2, size_t n);
int strcmp(const char *s1, const char *s2);
size_t strlen(const char *s);
void *memmove(void *dst, const void *src, size_t n);
int memcmp(const void *s1, const void *s2, size_t n);
char *strchr(const char *s, int c);
void __assert_fail(const char *expr, const char *file, unsigned int line, const char *func);
void *__memcpy_chk(void *dest, const void *src, size_t n, size_t destlen);

#ifdef __cplusplus
}
#endif

#endif /* MEMUTILS_H */
