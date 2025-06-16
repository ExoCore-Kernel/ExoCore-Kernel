#include <stddef.h>
void *memcpy(void *dst, const void *src, size_t n) {
    unsigned char *d = dst;
    const unsigned char *s = src;
    for (size_t i = 0; i < n; i++)
        d[i] = s[i];
    return dst;
}
void *memset(void *dst, int val, size_t n) {
    unsigned char *d = dst;
    for (size_t i = 0; i < n; i++)
        d[i] = (unsigned char)val;
    return dst;
}

int strncmp(const char *s1, const char *s2, size_t n) {
    for (size_t i = 0; i < n; i++) {
        unsigned char c1 = s1[i];
        unsigned char c2 = s2[i];
        if (c1 != c2)
            return c1 - c2;
        if (c1 == '\0')
            return 0;
    }
    return 0;
}
