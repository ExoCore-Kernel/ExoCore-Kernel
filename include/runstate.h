#ifndef RUNSTATE_H
#define RUNSTATE_H

#ifdef __cplusplus
extern "C" {
#endif

extern volatile const char *current_program;
extern volatile int current_user_app;
extern int debug_mode;
extern int mp_vga_output;

#ifdef __cplusplus
}
#endif

#endif /* RUNSTATE_H */
