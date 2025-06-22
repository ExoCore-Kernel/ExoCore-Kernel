#ifndef DEBUGLOG_H
#define DEBUGLOG_H

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

void debuglog_init(void);
void debuglog_char(char c);
void debuglog_flush(void);
void debuglog_dump_console(void);
void debuglog_hexdump(const void *data, size_t len);
void debuglog_memdump(const void *addr, size_t len);
const char *debuglog_buffer(void);
size_t debuglog_length(void);
void debuglog_save_file(void);
void debuglog_print_timestamp(void);

#ifdef __cplusplus
}
#endif

#endif /* DEBUGLOG_H */
