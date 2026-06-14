#include "bootlogo.h"
#include "embedded_logo.h"
#include "exoimg.h"
#include "framebuffer.h"
#include "mem.h"
#include "memutils.h"
#include "vfs.h"
#define LOGO_PATH "/boot/logo.exoimg"
int bootlogo_install_to_vfs(void){ if(embedded_logo_exoimg_len==0)return -1; if(!vfs_is_ready()&&vfs_init()!=0)return -1; vfs_mkdir("/boot"); int fd=vfs_open(LOGO_PATH,VFS_O_CREAT|VFS_O_RDWR|VFS_O_TRUNC); if(fd<0)return -1; long n=vfs_write(fd,embedded_logo_exoimg,embedded_logo_exoimg_len); vfs_close(fd); return n==(long)embedded_logo_exoimg_len?0:-1; }
int bootlogo_draw_from_vfs(void){ if(!framebuffer_enabled())return -1; vfs_stat_t st; if(vfs_stat(LOGO_PATH,&st)!=0||st.type!=VFS_TYPE_FILE||!st.size)return -1; unsigned char *buf=mem_alloc(st.size); if(!buf)return -1; int fd=vfs_open(LOGO_PATH,VFS_O_RDONLY); if(fd<0){mem_free(buf,st.size);return -1;} long got=vfs_read(fd,buf,st.size); vfs_close(fd); if(got!=(long)st.size){mem_free(buf,st.size);return -1;} exoimg_header_t h; if(exoimg_validate(buf,st.size,&h)!=0){mem_free(buf,st.size);return -1;} uint32_t x=framebuffer_width()>h.width?(framebuffer_width()-h.width)/2u:0; uint32_t y=framebuffer_height()>h.height?(framebuffer_height()-h.height)/2u:0; int ok=framebuffer_blit_rgba8888(x,y,h.width,h.height,buf+sizeof(exoimg_header_t),h.width*4u); mem_free(buf,st.size); return ok?0:-1; }
