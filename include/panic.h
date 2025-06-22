#ifndef PANIC_H
#define PANIC_H

#ifdef __cplusplus
extern "C" {
#endif

void panic(const char *msg);
void panic_with_context(const char *msg, uint64_t rip, int user);

#ifdef __cplusplus
}
#endif

#endif /* PANIC_H */
