#include "bootmode.h"
#include "console.h"
#include <stdint.h>
#include <stddef.h>
static int logs_visible=1;
static int progress_visible=1;
static uint32_t theme=BOOT_THEME_DARK;
static int token_eq(const char*p,const char*t){size_t i=0;while(t[i]&&p[i]&&p[i]==t[i])i++;return t[i]==0&&(p[i]==0||p[i]==' '||p[i]=='\t');}
void bootmode_init(void){ logs_visible = 1; progress_visible = 1; theme = BOOT_THEME_DARK; }
void bootmode_parse_cmdline(const char *cmd){ if(!cmd)return; for(const char*p=cmd;*p;p++){ if(token_eq(p,"nologs")||token_eq(p,"no-logs")) logs_visible=0; if(token_eq(p,"bootprogress")||token_eq(p,"progress")||token_eq(p,"splashprogress")) progress_visible=1; if(token_eq(p,"white")||token_eq(p,"theme=white")) theme=BOOT_THEME_WHITE; if(token_eq(p,"dark")||token_eq(p,"theme=dark")) theme=BOOT_THEME_DARK; }}
int bootmode_logs_visible(void){ return logs_visible; }
void bootmode_set_logs_visible(int visible){ logs_visible=visible?1:0; }
int bootmode_progress_visible(void){ return progress_visible; }
void bootmode_set_progress_visible(int visible){ progress_visible=visible?1:0; }
uint32_t bootmode_theme(void){ return theme; }
void bootmode_set_theme(uint32_t t){ theme=(t==BOOT_THEME_WHITE)?BOOT_THEME_WHITE:BOOT_THEME_DARK; }
uint8_t bootmode_fg(void){ return theme==BOOT_THEME_WHITE?VGA_BLACK:VGA_WHITE; }
uint8_t bootmode_bg(void){ return theme==BOOT_THEME_WHITE?VGA_WHITE:VGA_BLACK; }
