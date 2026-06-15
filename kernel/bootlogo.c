#include "bootlogo.h"
#include "embedded_logo.h"
#include "exoimg.h"
#include "framebuffer.h"
#include "mem.h"
#include "memutils.h"
#include "vfs.h"
#define LOGO_PATH "/boot/logo.exoimg"
#define BAR_WIDTH 630u
#define BAR_HEIGHT 18u
#define BAR_GAP 28u
static uint32_t logo_x, logo_y, logo_w, logo_h;
static int logo_position_valid;
static void px(uint32_t x,uint32_t y,uint8_t r,uint8_t g,uint8_t b){framebuffer_draw_pixel_rgb(x,y,r,g,b);} 
static void rect(uint32_t x,uint32_t y,uint32_t w,uint32_t h,uint8_t r,uint8_t g,uint8_t b){for(uint32_t yy=0;yy<h;yy++)for(uint32_t xx=0;xx<w;xx++)px(x+xx,y+yy,r,g,b);} 
static int rounded(uint32_t x,uint32_t y,uint32_t w,uint32_t h,uint32_t radius,uint32_t px0,uint32_t py0){uint32_t rx=px0<x+radius?x+radius-px0:px0>=x+w-radius?px0-(x+w-radius-1u):0;uint32_t ry=py0<y+radius?y+radius-py0:py0>=y+h-radius?py0-(y+h-radius-1u):0;return rx*rx+ry*ry<=radius*radius;}
int bootlogo_install_to_vfs(void){ if(embedded_logo_exoimg_len==0)return -1; if(!vfs_is_ready()&&vfs_init()!=0)return -1; vfs_mkdir("/boot"); int fd=vfs_open(LOGO_PATH,VFS_O_CREAT|VFS_O_RDWR|VFS_O_TRUNC); if(fd<0)return -1; long n=vfs_write(fd,embedded_logo_exoimg,embedded_logo_exoimg_len); vfs_close(fd); return n==(long)embedded_logo_exoimg_len?0:-1; }
int bootlogo_draw_from_vfs(void){ if(!framebuffer_enabled())return -1; vfs_stat_t st; if(vfs_stat(LOGO_PATH,&st)!=0||st.type!=VFS_TYPE_FILE||!st.size)return -1; unsigned char *buf=mem_alloc(st.size); if(!buf)return -1; int fd=vfs_open(LOGO_PATH,VFS_O_RDONLY); if(fd<0){mem_free(buf,st.size);return -1;} long got=vfs_read(fd,buf,st.size); vfs_close(fd); if(got!=(long)st.size){mem_free(buf,st.size);return -1;} exoimg_header_t h; if(exoimg_validate(buf,st.size,&h)!=0){mem_free(buf,st.size);return -1;} uint32_t total_h=h.height+BAR_GAP+BAR_HEIGHT; logo_x=framebuffer_width()>h.width?(framebuffer_width()-h.width)/2u:0; logo_y=framebuffer_height()>total_h?(framebuffer_height()-total_h)/2u:0; logo_w=h.width; logo_h=h.height; logo_position_valid=1; int ok=framebuffer_blit_rgba8888(logo_x,logo_y,h.width,h.height,buf+sizeof(exoimg_header_t),h.width*4u); mem_free(buf,st.size); return ok?0:-1; }
void bootlogo_draw_progress(uint32_t percent){ if(!framebuffer_enabled())return; if(percent>100u)percent=100u; uint32_t w=BAR_WIDTH; uint32_t maxw=framebuffer_width()>40u?framebuffer_width()-40u:framebuffer_width(); if(w>maxw)w=maxw; uint32_t x=framebuffer_width()>w?(framebuffer_width()-w)/2u:0; uint32_t y=logo_position_valid?logo_y+logo_h+BAR_GAP:(framebuffer_height()>BAR_HEIGHT?(framebuffer_height()-BAR_HEIGHT)/2u:0); uint32_t fill=(w*percent)/100u; uint32_t radius=BAR_HEIGHT/2u; for(uint32_t yy=0;yy<BAR_HEIGHT;yy++)for(uint32_t xx=0;xx<w;xx++){uint32_t sx=x+xx,sy=y+yy;if(!rounded(x,y,w,BAR_HEIGHT,radius,sx,sy))continue;uint8_t shade=(uint8_t)(34u+(yy*28u)/BAR_HEIGHT); if(xx<fill)shade=(uint8_t)(214u+(yy*28u)/BAR_HEIGHT); px(sx,sy,shade,shade,shade);} rect(x+2u,y+2u,w>4u?w-4u:0u,1u,95,95,95); }
