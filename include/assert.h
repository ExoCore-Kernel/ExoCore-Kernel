#ifndef EXOCORE_ASSERT_H
#define EXOCORE_ASSERT_H

/*
 * Freestanding assert for kernel/cross builds.
 * Provides the standard assert() interface without requiring host libc headers.
 */
#if defined(NDEBUG)
#define assert(ignore) ((void)0)
#else
#define assert(expr) ((void)((expr) ? 0 : __builtin_trap()))
#endif

#endif /* EXOCORE_ASSERT_H */
