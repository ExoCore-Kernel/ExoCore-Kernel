#ifndef EXOIMG_H
#define EXOIMG_H
#include <stddef.h>
#include <stdint.h>
#define EXOIMG_FORMAT_RGBA8888 1u
typedef struct { char magic[8]; uint32_t width; uint32_t height; uint32_t format; uint32_t data_size; } exoimg_header_t;
int exoimg_validate(const void *data, size_t size, exoimg_header_t *out);
#endif
