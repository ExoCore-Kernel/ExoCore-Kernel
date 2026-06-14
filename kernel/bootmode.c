#include "bootmode.h"
#include "console.h"
#include <string.h>
static int logs_visible = 1;
static uint32_t theme = BOOT_THEME_DARK;
void bootmode_init(void){ logs_visible = 1; theme = BOOT_THEME_DARK; }
static int token_eq(const char *p,const char *tok){size_t n=strlen(tok);return strncmp(p,tok,n)==0 && (p[n]==0||p[n]==' '||p[n]=='\t');}
void bootmode_parse_cmdline(const char *cmd){ if(!cmd)return; for(const char*p=cmd;*p;p++){ if(token_eq(p,"nologs")||token_eq(p,"no-logs")) logs_visible=0; if(token_eq(p,"white")||token_eq(p,"theme=white")) theme=BOOT_THEME_WHITE; if(token_eq(p,"dark")||token_eq(p,"theme=dark")) theme=BOOT_THEME_DARK; }}
int bootmode_logs_visible(void){ return logs_visible; }
void bootmode_set_logs_visible(int visible){ logs_visible=visible?1:0; }
uint32_t bootmode_theme(void){ return theme; }
void bootmode_set_theme(uint32_t t){ theme=(t==BOOT_THEME_WHITE)?BOOT_THEME_WHITE:BOOT_THEME_DARK; }
uint8_t bootmode_fg(void){ return theme==BOOT_THEME_WHITE?VGA_BLACK:VGA_WHITE; }
uint8_t bootmode_bg(void){ return theme==BOOT_THEME_WHITE?VGA_WHITE:VGA_BLACK; }
