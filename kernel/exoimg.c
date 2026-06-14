#include "exoimg.h"
#include "memutils.h"
#define EXOIMG_MAX_DIM 4096u
#define EXOIMG_MAX_BYTES (32u*1024u*1024u)
static uint32_t le32(uint32_t v){ const unsigned char*p=(const unsigned char*)&v; return (uint32_t)p[0]|((uint32_t)p[1]<<8)|((uint32_t)p[2]<<16)|((uint32_t)p[3]<<24); }
int exoimg_validate(const void *data, size_t size, exoimg_header_t *out){ if(!data||size<sizeof(exoimg_header_t))return -1; exoimg_header_t h; memcpy(&h,data,sizeof(h)); if(memcmp(h.magic,"EXOIMG1\0",8)!=0)return -1; h.width=le32(h.width); h.height=le32(h.height); h.format=le32(h.format); h.data_size=le32(h.data_size); if(!h.width||!h.height||h.width>EXOIMG_MAX_DIM||h.height>EXOIMG_MAX_DIM)return -1; if(h.format!=EXOIMG_FORMAT_RGBA8888)return -1; uint64_t expected=(uint64_t)h.width*h.height*4u; if(expected>EXOIMG_MAX_BYTES||h.data_size!=(uint32_t)expected)return -1; if(size<sizeof(exoimg_header_t)+(size_t)h.data_size)return -1; if(out)*out=h; return 0; }
