#include <stdint.h>
#include <stddef.h>
#include "console.h"
#include "ps2kbd.h"

#define MAX_INPUT 64
#define HIST_SIZE 8

static char history[HIST_SIZE][MAX_INPUT];
static int hist_count = 0;

static size_t strlen_s(const char *s) { size_t n=0; while(s[n]) n++; return n; }
static void strcpy_s(char *d, const char *s) { while((*d++ = *s++)); }
static int strncmp_s(const char *a, const char *b, size_t n) {
    for(size_t i=0;i<n;i++) { unsigned char c1=a[i], c2=b[i]; if(c1!=c2) return c1-c2; if(!c1) return 0; }
    return 0; }

static void print_prompt(void){ console_puts("exocore$ "); }

static void add_history(const char *cmd){
    if(strlen_s(cmd)==0) return;
    if(hist_count < HIST_SIZE){ strcpy_s(history[hist_count], cmd); hist_count++; }
    else { for(int i=1;i<HIST_SIZE;i++) strcpy_s(history[i-1], history[i]); strcpy_s(history[HIST_SIZE-1], cmd); }
}

static void show_history(int idx, char *buf, int *len){
    for(int i=0;i<*len;i++) console_backspace();
    strcpy_s(buf, history[idx]);
    *len = (int)strlen_s(buf);
    console_puts(buf);
}

static void handle_command(const char *cmd){
    if(!strncmp_s(cmd,"help",4)){
        console_puts("Available: help, clear, halt\n");
    } else if(!strncmp_s(cmd,"clear",5)){
        console_clear();
    } else if(!strncmp_s(cmd,"halt",4)){
        console_puts("Halting...\n");
        for(;;) __asm__("hlt");
    } else if(strlen_s(cmd)>0){
        console_puts("Unknown command: ");
        console_puts(cmd);
        console_putc('\n');
    }
}

void _start(){
    console_clear();
    console_puts("[shell] ExoCore shell\n");
    char buf[MAX_INPUT];
    int len=0; int hist_nav=0;
    for(;;){
        print_prompt();
        len=0; buf[0]='\0';
        hist_nav = hist_count;
        for(;;){
            char c = console_getc();
            if(c=='\n'){ console_putc('\n'); break; }
            else if(c=='\b'){ if(len>0){ len--; console_backspace(); } }
            else if((unsigned char)c==0x80){
                if(hist_count){
                    if(hist_nav>0) hist_nav--;
                    show_history(hist_nav, buf, &len);
                }
            }
            else if((unsigned char)c==0x81){
                if(hist_count){
                    if(hist_nav < hist_count-1) hist_nav++;
                    else {
                        hist_nav = hist_count;
                        for(int i=0;i<len;i++) console_backspace();
                        len=0; buf[0]='\0';
                        continue;
                    }
                    show_history(hist_nav, buf, &len);
                }
            }
            else { if(len<MAX_INPUT-1){ buf[len++]=c; buf[len]='\0'; console_putc(c); } }
        }
        buf[len]='\0';
        add_history(buf);
        handle_command(buf);
    }
}
