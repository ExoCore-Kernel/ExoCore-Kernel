#include "launchd.h"
#include "console.h"
#include "elf.h"
#include "fs.h"
#include "memutils.h"
#include "multiboot.h"
#include "proc.h"
#include "serial.h"
#include "vfs.h"
#include <stdint.h>
#include <stddef.h>
#include <string.h>

#define LAUNCHD_IMAGE_NAME "userland.img"
#define LAUNCHD_BINARY_PATH "/launchd.elf"
#define LAUNCHD_CONFIG_PATH "/launchd.cfg"
#define SHELLD_BINARY_PATH "/shelld.elf"
#define LAUNCHD_MAX_FILE (1024U * 1024U)
#define EXO_ALLOW_FAKE_LAUNCHD 0

static void launchd_log(const char *msg) {
    console_puts(msg);
    serial_write(msg);
}

static int module_name_matches(const char *module, const char *needle) {
    if (!module || !needle)
        return 0;
    const char *last = module;
    for (const char *p = module; *p; ++p) {
        if (*p == '/' || *p == ' ')
            last = p + 1;
    }
    return strcmp(last, needle) == 0 || strstr(module, needle) != 0;
}

static int copy_fat_file_to_vfs(const char *path) {
    int src = fs_open(path, 1);
    if (src < 0)
        return -1;
    long file_size = fs_file_size(src);
    if (file_size <= 0 || file_size > (long)LAUNCHD_MAX_FILE) {
        fs_close(src);
        return -1;
    }
    int dst = vfs_open(path, VFS_O_CREAT | VFS_O_RDWR | VFS_O_TRUNC);
    if (dst < 0) {
        fs_close(src);
        return -1;
    }
    unsigned char buf[512];
    long remaining = file_size;
    while (remaining > 0) {
        size_t chunk = remaining > (long)sizeof(buf) ? sizeof(buf) : (size_t)remaining;
        long got = fs_read_fd(src, buf, chunk);
        if (got != (long)chunk || vfs_write(dst, buf, chunk) != (long)chunk) {
            vfs_close(dst);
            fs_close(src);
            return -1;
        }
        remaining -= got;
    }
    vfs_close(dst);
    fs_close(src);
    return 0;
}

static int read_vfs_file_to_process(int pid, const char *path, void **out_data, size_t *out_size) {
    vfs_stat_t st;
    if (!out_data || !out_size || vfs_stat(path, &st) != 0 || st.type != VFS_TYPE_FILE || st.size == 0)
        return -1;
    void *data = proc_alloc(pid, st.size);
    if (!data)
        return -1;
    int fd = vfs_open(path, VFS_O_RDONLY);
    if (fd < 0) {
        proc_free(pid, data);
        return -1;
    }
    long got = vfs_read(fd, data, st.size);
    vfs_close(fd);
    if (got != (long)st.size) {
        proc_free(pid, data);
        return -1;
    }
    *out_data = data;
    *out_size = st.size;
    return 0;
}

int launchd_boot(multiboot_info_t *mbi) {
    if (!mbi || !(mbi->flags & (1 << 3)) || mbi->mods_count == 0)
        return -1;

    multiboot_module_t *mods = (multiboot_module_t*)(uintptr_t)mbi->mods_addr;
    const void *image = NULL;
    size_t image_size = 0;
    for (uint32_t i = 0; i < mbi->mods_count; ++i) {
        const char *name = (const char*)(uintptr_t)mods[i].string;
        if (module_name_matches(name, LAUNCHD_IMAGE_NAME)) {
            image = (const void*)(uintptr_t)mods[i].mod_start;
            image_size = (size_t)(mods[i].mod_end - mods[i].mod_start);
            break;
        }
    }

    if (!image || image_size == 0) {
        launchd_log("launchd: userland FAT32 image not found\n");
        return -1;
    }

    launchd_log("kernel: mounting fat32 now\n");
    fs_mount((void*)image, image_size);
    if (!fs_is_fat32()) {
        launchd_log("launchd: userland image is not FAT32\n");
        return -1;
    }
    launchd_log("kernel: mounted fat32 successfully!\n");

    if (!vfs_is_ready() && vfs_init() != 0)
        return -1;
    if (copy_fat_file_to_vfs(LAUNCHD_BINARY_PATH) != 0 ||
        copy_fat_file_to_vfs(LAUNCHD_CONFIG_PATH) != 0 ||
        copy_fat_file_to_vfs(SHELLD_BINARY_PATH) != 0) {
        launchd_log("launchd: failed to mirror FAT32 userland into VFS\n");
        return -1;
    }

    int launchd_pid = proc_create("launchd", 0);
    if (launchd_pid != 1 || proc_set_current(launchd_pid) != 0) {
        launchd_log("launchd: failed to create pid 1\n");
        return -1;
    }

    launchd_log("kernel: finding elf executable\n");
    void *launchd_file = NULL;
    size_t launchd_size = 0;
    if (read_vfs_file_to_process(launchd_pid, LAUNCHD_BINARY_PATH, &launchd_file, &launchd_size) != 0) {
        launchd_log("launchd: failed to read /launchd.elf through VFS\n");
        return -1;
    }
    if (elf_validate(launchd_file, launchd_size) != 0) {
        launchd_log("launchd: invalid /launchd.elf\n");
        return -1;
    }
    launchd_log("kernel: found elf magic header!\n");

    elf_image_t loaded;
    if (elf_load_process_image(launchd_pid, launchd_file, launchd_size, &loaded) != 0 ||
        proc_attach_image(launchd_pid, LAUNCHD_BINARY_PATH, &loaded) != 0) {
        launchd_log("launchd: failed to load executable image\n");
        return -1;
    }

    launchd_log("kernel: handing over to launchd as PID 1\n");
    return proc_start_flat(launchd_pid);
}
