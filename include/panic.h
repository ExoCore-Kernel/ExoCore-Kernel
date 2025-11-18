#ifndef PANIC_H
#define PANIC_H

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

void panic(const char *msg);
void panic_with_context(const char *msg, uint64_t rip, int user);
void panic_set_micropython_details(const char *type, const char *msg,
                                   const char *file, size_t line,
                                   const char *function_name);

#ifdef __cplusplus
}
#endif

#endif /* PANIC_H */
