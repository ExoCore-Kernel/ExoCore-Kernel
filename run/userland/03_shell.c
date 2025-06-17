#include <stdint.h>
#include <stddef.h>
#include "console.h"
#include "ps2kbd.h"
#include "mem.h"

#define MAX_INPUT 64
#define HIST_SIZE 8

static char history[HIST_SIZE][MAX_INPUT];
static int hist_count = 0;
static int app_id = -1;

static const char *boot_order[8] = {
    "run00_kernel_tester",
    "run_example",
    "run_memtest",
    "run02_kernel_tests",
    "run03_shell"
};
static int boot_count = 5;

static size_t strlen_s(const char *s) { size_t n=0; while(s[n]) n++; return n; }
static void strcpy_s(char *d, const char *s) { while((*d++ = *s++)); }
static int strncmp_s(const char *a, const char *b, size_t n) {
    for(size_t i=0;i<n;i++) { unsigned char c1=a[i], c2=b[i]; if(c1!=c2) return c1-c2; if(!c1) return 0; }
    return 0; }

static int atoi_s(const char *s){
    int v=0; while(*s>='0'&&*s<='9'){ v=v*10+(*s-'0'); s++; } return v;
}

static void print_prompt(void){ console_puts("exocore$ "); }

static void add_history(const char *cmd){
    if(strlen_s(cmd)==0) return;
    if(hist_count < HIST_SIZE){ strcpy_s(history[hist_count], cmd); hist_count++; }
    else { for(int i=1;i<HIST_SIZE;i++) strcpy_s(history[i-1], history[i]); strcpy_s(history[HIST_SIZE-1], cmd); }
}

static void gfx_test(void){
    volatile uint16_t *video = (uint16_t*)0xB8000;
    for(int c=0;c<16;c++){
        for(int i=0;i<80*25;i++)
            video[i] = ((uint16_t)VGA_ATTR(c, VGA_BLACK) << 8) | ' ';
        for(volatile int d=0; d<500000; d++);
    }
    for(int y=5;y<10;y++)
        for(int x=10;x<20;x++)
            video[y*80+x] = ((uint16_t)VGA_ATTR(VGA_LIGHT_RED, VGA_BLACK)<<8) | '#';
    for(int y=12;y<17;y++)
        for(int x=40;x<60;x++)
            video[y*80+x] = ((uint16_t)VGA_ATTR(VGA_LIGHT_GREEN, VGA_BLACK)<<8) | '*';
    for(volatile int d=0; d<1000000; d++);
    console_clear();
}

static void show_history(int idx, char *buf, int *len){
    for(int i=0;i<*len;i++) console_backspace();
    strcpy_s(buf, history[idx]);
    *len = (int)strlen_s(buf);
    console_puts(buf);
}

static void handle_command(const char *cmd){
    if(!strncmp_s(cmd,"gfxtst",6)){
        gfx_test();
    } else if(!strncmp_s(cmd,"elp",3)){
        console_puts("Commands: gfxtst, elp, order, chngeordr\n");
    } else if(!strncmp_s(cmd,"order",5)){
        for(int i=0;i<boot_count;i++){
            console_puts(boot_order[i]);
            console_putc('\n');
        }
    } else if(!strncmp_s(cmd,"chngeordr",9)){
        int idx=0;
        const char *p = cmd+9;
        while(*p && idx < 8){
            while(*p==' ') p++;
            if(!*p) break;
            const char *start=p;
            while(*p && *p!=' ') p++;
            int len = p-start;
            if(len>0){
                int l = (len<31)?len:31;
                char *tmp = mem_alloc_app(app_id, l+1);
                if(!tmp) continue;
                for(int i=0;i<l;i++) tmp[i]=start[i];
                tmp[l]='\0';
                boot_order[idx++] = tmp;
            }
        }
        boot_count = idx;
    } else if(strlen_s(cmd)>0){
        console_puts("Unknown command: ");
        console_puts(cmd);
        console_putc('\n');
    }
}

void _start(){
    console_clear();
    console_puts("[shell] ExoCore shell\n");
    app_id = mem_register_app(1);
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
            else if((unsigned char)c==0x80){ /* left arrow: prev command */
                if(hist_count){
                    if(hist_nav>0) hist_nav--;
                    show_history(hist_nav, buf, &len);
                }
            }
            else if((unsigned char)c==0x81){ /* right arrow: next command */
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
