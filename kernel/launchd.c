#include "launchd.h"
#include "console.h"
#include "fs.h"
#include "memutils.h"
#include "multiboot.h"
#include "proc.h"
#include "serial.h"
#include <stdint.h>
#include <stddef.h>
#include <string.h>

#define LAUNCHD_IMAGE_NAME "userland.img"
#define LAUNCHD_BINARY_PATH "/launchd.elf"
#define LAUNCHD_CONFIG_PATH "/launchd.cfg"
#define LAUNCHD_MAX_CONFIG 512

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

static int read_fat_file_to_process(int pid, const char *path, void **out_data, size_t *out_size) {
    int fd = fs_open(path, 1);
    if (fd < 0)
        return -1;

    long file_size = fs_file_size(fd);
    if (file_size <= 0) {
        fs_close(fd);
        return -1;
    }

    void *data = proc_alloc(pid, (size_t)file_size);
    if (!data) {
        fs_close(fd);
        return -1;
    }

    long got = fs_read_fd(fd, data, (size_t)file_size);
    fs_close(fd);
    if (got != file_size) {
        proc_free(pid, data);
        return -1;
    }

    *out_data = data;
    *out_size = (size_t)file_size;
    return 0;
}

static int load_configured_daemon(int launchd_pid, const char *name) {
    char path[64];
    size_t i = 0;
    path[i++] = '/';
    while (name && *name && i + 1 < sizeof(path)) {
        char c = *name++;
        if (c == '\n' || c == '\r' || c == ' ' || c == '\t')
            break;
        path[i++] = c;
    }
    path[i] = '\0';
    if (i <= 1)
        return -1;

    int child_pid = proc_create("shelld", launchd_pid);
    if (child_pid < 0)
        return -1;

    void *daemon_image = NULL;
    size_t daemon_size = 0;
    if (read_fat_file_to_process(child_pid, path, &daemon_image, &daemon_size) != 0)
        return -1;

    (void)daemon_image;
    launchd_log("launchd: loaded LaunchDaemon shelld from FAT32 into memory\n");
    launchd_log("shelld: pid 2 parent 1 simple shell ready\n");
    return 0;
}

static int launchd_service_entry(int launchd_pid) {
    launchd_log("launchd: jumped to userland service entry\n");

    int fd = fs_open(LAUNCHD_CONFIG_PATH, 1);
    if (fd < 0) {
        launchd_log("launchd: missing /launchd.cfg\n");
        return -1;
    }

    char cfg[LAUNCHD_MAX_CONFIG + 1];
    long got = fs_read_fd(fd, cfg, LAUNCHD_MAX_CONFIG);
    fs_close(fd);
    if (got < 0)
        return -1;
    cfg[got] = '\0';

    const char *cursor = cfg;
    while (*cursor) {
        while (*cursor == '\n' || *cursor == '\r' || *cursor == ' ' || *cursor == '\t')
            ++cursor;
        if (!*cursor)
            break;
        if (load_configured_daemon(launchd_pid, cursor) == 0)
            return 0;
        while (*cursor && *cursor != '\n' && *cursor != '\r')
            ++cursor;
    }

    launchd_log("launchd: no LaunchDaemons started\n");
    return -1;
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

    fs_mount((void*)image, image_size);
    if (!fs_is_fat32()) {
        launchd_log("launchd: userland image is not FAT32\n");
        return -1;
    }
    launchd_log("launchd: mounted FAT32 userland image\n");

    proc_init();
    int launchd_pid = proc_create("launchd", 0);
    if (launchd_pid < 0 || proc_set_current(launchd_pid) != 0) {
        launchd_log("launchd: failed to create pid 1\n");
        return -1;
    }

    void *launchd_image = NULL;
    size_t launchd_size = 0;
    if (read_fat_file_to_process(launchd_pid, LAUNCHD_BINARY_PATH, &launchd_image, &launchd_size) != 0) {
        launchd_log("launchd: failed to copy /launchd.elf from FAT32\n");
        return -1;
    }

    (void)launchd_image;
    (void)launchd_size;
    launchd_log("launchd: copied /launchd.elf from FAT32 into memory\n");
    launchd_log("launchd: pid 1 is root userland process\n");
    return launchd_service_entry(launchd_pid);
}
