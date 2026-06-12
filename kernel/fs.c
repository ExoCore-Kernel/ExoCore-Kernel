#include "fs.h"
#include "memutils.h"
#include "console.h"

#define FS_MODE_NONE 0
#define FS_MODE_RAW  1
#define FS_MODE_FAT32 2

#define FS_O_RDONLY 0x01
#define FS_O_WRONLY 0x02
#define FS_O_RDWR   0x03
#define FS_O_CREAT  0x10
#define FS_O_TRUNC  0x20
#define FS_O_APPEND 0x40

#define FS_SEEK_SET 0
#define FS_SEEK_CUR 1
#define FS_SEEK_END 2

#define FAT32_EOC 0x0FFFFFF8U
#define FAT32_EOC_MARK 0x0FFFFFFFU
#define FAT32_ATTR_DIRECTORY 0x10
#define FAT32_ATTR_ARCHIVE 0x20

typedef struct {
    uint16_t bytes_per_sector;
    uint8_t sectors_per_cluster;
    uint16_t reserved_sectors;
    uint8_t fat_count;
    uint32_t sectors_per_fat;
    uint32_t total_sectors;
    uint32_t root_cluster;
    uint32_t first_data_sector;
    uint32_t total_clusters;
} fat32_info_t;

typedef struct {
    int used;
    uint32_t dir_entry_offset;
    uint32_t first_cluster;
    uint32_t size;
    uint32_t pos;
    int flags;
} fs_open_t;

static unsigned char *fs_data = NULL;
static size_t fs_size = 0;
static int fs_mode = FS_MODE_NONE;
static fat32_info_t fat;
static fs_open_t open_files[FS_MAX_OPEN];

static uint16_t rd16(const unsigned char *p) {
    return (uint16_t)p[0] | ((uint16_t)p[1] << 8);
}

static uint32_t rd32(const unsigned char *p) {
    return (uint32_t)p[0] | ((uint32_t)p[1] << 8) | ((uint32_t)p[2] << 16) | ((uint32_t)p[3] << 24);
}

static void wr16(unsigned char *p, uint16_t v) {
    p[0] = (unsigned char)v;
    p[1] = (unsigned char)(v >> 8);
}

static void wr32(unsigned char *p, uint32_t v) {
    p[0] = (unsigned char)v;
    p[1] = (unsigned char)(v >> 8);
    p[2] = (unsigned char)(v >> 16);
    p[3] = (unsigned char)(v >> 24);
}

static int range_ok(size_t off, size_t len) {
    return fs_data && off <= fs_size && len <= fs_size - off;
}

static int fat32_parse(void) {
    if (!range_ok(0, 512))
        return -1;
    const unsigned char *b = fs_data;
    if (b[510] != 0x55 || b[511] != 0xAA)
        return -1;

    uint16_t bps = rd16(b + 11);
    uint8_t spc = b[13];
    uint16_t reserved = rd16(b + 14);
    uint8_t fats = b[16];
    uint16_t root_entries = rd16(b + 17);
    uint16_t total16 = rd16(b + 19);
    uint16_t fat16 = rd16(b + 22);
    uint32_t total32 = rd32(b + 32);
    uint32_t fat32 = rd32(b + 36);
    uint32_t root_cluster = rd32(b + 44);

    if (bps == 0 || (bps & (bps - 1)) != 0 || bps < 512 || spc == 0 ||
        reserved == 0 || fats == 0 || root_entries != 0 || total16 != 0 ||
        fat16 != 0 || fat32 == 0 || root_cluster < 2)
        return -1;

    uint32_t total = total32;
    uint32_t first_data = (uint32_t)reserved + (uint32_t)fats * fat32;
    if (total <= first_data)
        return -1;
    if ((uint64_t)total * bps > fs_size)
        return -1;

    uint32_t clusters = (total - first_data) / spc;
    if (clusters == 0 || b[82] != 'F' || b[83] != 'A' || b[84] != 'T' || b[85] != '3' || b[86] != '2')
        return -1;

    fat.bytes_per_sector = bps;
    fat.sectors_per_cluster = spc;
    fat.reserved_sectors = reserved;
    fat.fat_count = fats;
    fat.sectors_per_fat = fat32;
    fat.total_sectors = total;
    fat.root_cluster = root_cluster;
    fat.first_data_sector = first_data;
    fat.total_clusters = clusters;
    return 0;
}

static size_t cluster_offset(uint32_t cluster) {
    if (cluster < 2 || cluster >= fat.total_clusters + 2)
        return fs_size;
    uint32_t sector = fat.first_data_sector + (cluster - 2) * fat.sectors_per_cluster;
    return (size_t)sector * fat.bytes_per_sector;
}

static size_t cluster_size(void) {
    return (size_t)fat.bytes_per_sector * fat.sectors_per_cluster;
}

static size_t fat_entry_offset(uint32_t cluster, uint32_t fat_index) {
    return ((size_t)fat.reserved_sectors + (size_t)fat_index * fat.sectors_per_fat) * fat.bytes_per_sector +
           (size_t)cluster * 4;
}

static uint32_t fat_get(uint32_t cluster) {
    size_t off = fat_entry_offset(cluster, 0);
    if (!range_ok(off, 4))
        return FAT32_EOC_MARK;
    return rd32(fs_data + off) & 0x0FFFFFFF;
}

static int fat_set(uint32_t cluster, uint32_t value) {
    for (uint32_t i = 0; i < fat.fat_count; ++i) {
        size_t off = fat_entry_offset(cluster, i);
        if (!range_ok(off, 4))
            return -1;
        wr32(fs_data + off, value & 0x0FFFFFFF);
    }
    return 0;
}

static int is_eoc(uint32_t value) {
    return value >= FAT32_EOC;
}

static uint32_t alloc_cluster(void) {
    for (uint32_t c = 2; c < fat.total_clusters + 2; ++c) {
        if (fat_get(c) == 0) {
            if (fat_set(c, FAT32_EOC_MARK) != 0)
                return 0;
            size_t off = cluster_offset(c);
            if (!range_ok(off, cluster_size()))
                return 0;
            memset(fs_data + off, 0, cluster_size());
            return c;
        }
    }
    return 0;
}

static uint32_t chain_cluster(uint32_t first, uint32_t index, int grow) {
    if (first < 2)
        return 0;
    uint32_t cur = first;
    for (uint32_t i = 0; i < index; ++i) {
        uint32_t next = fat_get(cur);
        if (is_eoc(next)) {
            if (!grow)
                return 0;
            next = alloc_cluster();
            if (!next || fat_set(cur, next) != 0)
                return 0;
        }
        if (next < 2 || next >= fat.total_clusters + 2)
            return 0;
        cur = next;
    }
    return cur;
}

static void free_chain(uint32_t first) {
    uint32_t cur = first;
    while (cur >= 2 && cur < fat.total_clusters + 2) {
        uint32_t next = fat_get(cur);
        fat_set(cur, 0);
        if (is_eoc(next))
            break;
        cur = next;
    }
}

static int upper_char(int c) {
    if (c >= 'a' && c <= 'z')
        return c - ('a' - 'A');
    return c;
}

static int make_short_name(const char *path, unsigned char out[11]) {
    if (!path || !*path)
        return -1;
    while (*path == '/')
        ++path;
    if (!*path)
        return -1;
    memset(out, ' ', 11);

    int name_len = 0;
    int ext_len = 0;
    int in_ext = 0;
    for (const char *p = path; *p; ++p) {
        unsigned char ch = (unsigned char)*p;
        if (ch == '/')
            return -1;
        if (ch == '.') {
            if (in_ext)
                return -1;
            in_ext = 1;
            continue;
        }
        if (ch <= ' ' || ch == '"' || ch == '*' || ch == '+' || ch == ',' || ch == ':' ||
            ch == ';' || ch == '<' || ch == '=' || ch == '>' || ch == '?' || ch == '[' ||
            ch == '\\' || ch == ']' || ch == '|')
            return -1;
        if (!in_ext) {
            if (name_len >= 8)
                return -1;
            out[name_len++] = (unsigned char)upper_char(ch);
        } else {
            if (ext_len >= 3)
                return -1;
            out[8 + ext_len++] = (unsigned char)upper_char(ch);
        }
    }
    return name_len > 0 ? 0 : -1;
}

static int short_name_eq(const unsigned char *entry, const unsigned char name[11]) {
    for (int i = 0; i < 11; ++i) {
        if (entry[i] != name[i])
            return 0;
    }
    return 1;
}

static int find_dir_entry(const unsigned char name[11], uint32_t *entry_off, uint32_t *free_off) {
    size_t csz = cluster_size();
    uint32_t c = fat.root_cluster;
    if (free_off)
        *free_off = 0;
    while (c >= 2 && c < fat.total_clusters + 2) {
        size_t off = cluster_offset(c);
        if (!range_ok(off, csz))
            return -1;
        for (size_t pos = 0; pos + 32 <= csz; pos += 32) {
            unsigned char *e = fs_data + off + pos;
            if (e[0] == 0x00) {
                if (free_off && !*free_off)
                    *free_off = (uint32_t)(off + pos);
                return -1;
            }
            if (e[0] == 0xE5) {
                if (free_off && !*free_off)
                    *free_off = (uint32_t)(off + pos);
                continue;
            }
            if ((e[11] & 0x0F) == 0x0F)
                continue;
            if ((e[11] & FAT32_ATTR_DIRECTORY) != 0)
                continue;
            if (short_name_eq(e, name)) {
                if (entry_off)
                    *entry_off = (uint32_t)(off + pos);
                return 0;
            }
        }
        uint32_t next = fat_get(c);
        if (is_eoc(next))
            break;
        c = next;
    }
    return -1;
}

static uint32_t entry_first_cluster(unsigned char *e) {
    return ((uint32_t)rd16(e + 20) << 16) | rd16(e + 26);
}

static void entry_set_first_cluster(unsigned char *e, uint32_t c) {
    wr16(e + 20, (uint16_t)(c >> 16));
    wr16(e + 26, (uint16_t)c);
}

void fs_mount(void *storage, size_t size) {
    fs_data = (unsigned char *)storage;
    fs_size = size;
    fs_mode = fs_data ? FS_MODE_RAW : FS_MODE_NONE;
    memset(&fat, 0, sizeof(fat));
    memset(open_files, 0, sizeof(open_files));

    if (fs_data && fat32_parse() == 0) {
        fs_mode = FS_MODE_FAT32;
        console_puts("fs_mount FAT32 bytes=");
    } else {
        console_puts("fs_mount raw bytes=");
    }
    console_udec(size);
    console_putc('\n');
}

size_t fs_read(size_t offset, void *buf, size_t len) {
    if (!buf || !range_ok(offset, 0))
        return 0;
    if (offset + len > fs_size)
        len = fs_size - offset;
    memcpy(buf, fs_data + offset, len);
    return len;
}

size_t fs_write(size_t offset, const void *data, size_t len) {
    if (!data || !range_ok(offset, 0))
        return 0;
    if (offset + len > fs_size)
        len = fs_size - offset;
    memcpy(fs_data + offset, data, len);
    return len;
}

int fs_is_mounted(void) {
    return fs_mode != FS_MODE_NONE;
}

int fs_is_fat32(void) {
    return fs_mode == FS_MODE_FAT32;
}

size_t fs_capacity(void) {
    return fs_size;
}

int fs_open(const char *path, int flags) {
    if (fs_mode != FS_MODE_FAT32)
        return -1;
    unsigned char name[11];
    if (make_short_name(path, name) != 0)
        return -1;

    uint32_t entry_off = 0;
    uint32_t free_off = 0;
    int found = find_dir_entry(name, &entry_off, &free_off) == 0;
    if (!found) {
        if (!(flags & FS_O_CREAT) || !free_off)
            return -1;
        entry_off = free_off;
        unsigned char *e = fs_data + entry_off;
        memset(e, 0, 32);
        memcpy(e, name, 11);
        e[11] = FAT32_ATTR_ARCHIVE;
        wr32(e + 28, 0);
        entry_set_first_cluster(e, 0);
    } else if (flags & FS_O_TRUNC) {
        unsigned char *e = fs_data + entry_off;
        uint32_t first = entry_first_cluster(e);
        if (first)
            free_chain(first);
        entry_set_first_cluster(e, 0);
        wr32(e + 28, 0);
    }

    for (int fd = 0; fd < FS_MAX_OPEN; ++fd) {
        if (!open_files[fd].used) {
            unsigned char *e = fs_data + entry_off;
            open_files[fd].used = 1;
            open_files[fd].dir_entry_offset = entry_off;
            open_files[fd].first_cluster = entry_first_cluster(e);
            open_files[fd].size = rd32(e + 28);
            open_files[fd].flags = flags;
            open_files[fd].pos = (flags & FS_O_APPEND) ? open_files[fd].size : 0;
            return fd;
        }
    }
    return -1;
}

long fs_read_fd(int fd, void *buf, size_t len) {
    if (fs_mode != FS_MODE_FAT32 || fd < 0 || fd >= FS_MAX_OPEN || !open_files[fd].used || !buf)
        return -1;
    fs_open_t *of = &open_files[fd];
    if (of->pos >= of->size || len == 0)
        return 0;
    if (len > of->size - of->pos)
        len = of->size - of->pos;

    size_t done = 0;
    size_t csz = cluster_size();
    unsigned char *out = (unsigned char *)buf;
    while (done < len) {
        uint32_t cluster_index = of->pos / csz;
        uint32_t cluster_pos = of->pos % csz;
        uint32_t c = chain_cluster(of->first_cluster, cluster_index, 0);
        if (!c)
            break;
        size_t off = cluster_offset(c) + cluster_pos;
        size_t chunk = csz - cluster_pos;
        if (chunk > len - done)
            chunk = len - done;
        if (!range_ok(off, chunk))
            break;
        memcpy(out + done, fs_data + off, chunk);
        of->pos += (uint32_t)chunk;
        done += chunk;
    }
    return (long)done;
}

long fs_write_fd(int fd, const void *buf, size_t len) {
    if (fs_mode != FS_MODE_FAT32 || fd < 0 || fd >= FS_MAX_OPEN || !open_files[fd].used || !buf)
        return -1;
    fs_open_t *of = &open_files[fd];
    if ((of->flags & FS_O_WRONLY) == 0 && (of->flags & FS_O_RDWR) != FS_O_RDWR)
        return -1;

    size_t done = 0;
    size_t csz = cluster_size();
    const unsigned char *in = (const unsigned char *)buf;
    if (!of->first_cluster) {
        of->first_cluster = alloc_cluster();
        if (!of->first_cluster)
            return -1;
    }

    while (done < len) {
        uint32_t cluster_index = of->pos / csz;
        uint32_t cluster_pos = of->pos % csz;
        uint32_t c = chain_cluster(of->first_cluster, cluster_index, 1);
        if (!c)
            break;
        size_t off = cluster_offset(c) + cluster_pos;
        size_t chunk = csz - cluster_pos;
        if (chunk > len - done)
            chunk = len - done;
        if (!range_ok(off, chunk))
            break;
        memcpy(fs_data + off, in + done, chunk);
        of->pos += (uint32_t)chunk;
        if (of->pos > of->size)
            of->size = of->pos;
        done += chunk;
    }

    unsigned char *e = fs_data + of->dir_entry_offset;
    entry_set_first_cluster(e, of->first_cluster);
    wr32(e + 28, of->size);
    return (long)done;
}

long fs_lseek_fd(int fd, long offset, int whence) {
    if (fs_mode != FS_MODE_FAT32 || fd < 0 || fd >= FS_MAX_OPEN || !open_files[fd].used)
        return -1;
    fs_open_t *of = &open_files[fd];
    long base;
    if (whence == FS_SEEK_SET)
        base = 0;
    else if (whence == FS_SEEK_CUR)
        base = (long)of->pos;
    else if (whence == FS_SEEK_END)
        base = (long)of->size;
    else
        return -1;
    long next = base + offset;
    if (next < 0)
        return -1;
    of->pos = (uint32_t)next;
    return next;
}

int fs_close(int fd) {
    if (fd < 0 || fd >= FS_MAX_OPEN || !open_files[fd].used)
        return -1;
    memset(&open_files[fd], 0, sizeof(open_files[fd]));
    return 0;
}

long fs_file_size(int fd) {
    if (fs_mode != FS_MODE_FAT32 || fd < 0 || fd >= FS_MAX_OPEN || !open_files[fd].used)
        return -1;
    return (long)open_files[fd].size;
}
