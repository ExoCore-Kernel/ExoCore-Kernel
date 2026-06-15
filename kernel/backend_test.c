#include "backend_test.h"
#include "console.h"
#include "serial.h"
#include "vfs.h"
#include "fs.h"
#include "proc.h"
#include "memctx.h"
#include "mem.h"
#include "memutils.h"
#include "elf.h"
#include "launchd.h"
#include "bootmode.h"
#include "exoimg.h"

static void test_log(const char *msg) {
    console_puts(msg);
}

static int expect(int condition, const char *name) {
    if (condition) {
        test_log("[backend-test] PASS ");
        test_log(name);
        test_log("\n");
        return 0;
    }
    test_log("[backend-test] FAIL ");
    test_log(name);
    test_log("\n");
    return -1;
}


static void put16(unsigned char *p, uint16_t v) {
    p[0] = (unsigned char)v;
    p[1] = (unsigned char)(v >> 8);
}

static void put32(unsigned char *p, uint32_t v) {
    p[0] = (unsigned char)v;
    p[1] = (unsigned char)(v >> 8);
    p[2] = (unsigned char)(v >> 16);
    p[3] = (unsigned char)(v >> 24);
}

static void make_test_fat32(unsigned char *img, size_t bytes) {
    memset(img, 0, bytes);
    img[0] = 0xEB;
    img[1] = 0x58;
    img[2] = 0x90;
    img[3] = 'E'; img[4] = 'X'; img[5] = 'O'; img[6] = 'F';
    img[7] = 'A'; img[8] = 'T'; img[9] = '3'; img[10] = '2';
    put16(img + 11, 512);
    img[13] = 1;
    put16(img + 14, 1);
    img[16] = 1;
    img[21] = 0xF8;
    put32(img + 32, (uint32_t)(bytes / 512));
    put32(img + 36, 1);
    put32(img + 44, 2);
    img[82] = 'F'; img[83] = 'A'; img[84] = 'T'; img[85] = '3'; img[86] = '2';
    img[510] = 0x55;
    img[511] = 0xAA;
    unsigned char *fat = img + 512;
    put32(fat + 0, 0x0FFFFFF8);
    put32(fat + 4, 0x0FFFFFFF);
    put32(fat + 8, 0x0FFFFFFF);
}

static int test_boot_modes_and_exoimg(void) {
    bootmode_init();
    bootmode_parse_cmdline("white nologs bootprogress");
    if (expect(bootmode_theme() == BOOT_THEME_WHITE && !bootmode_logs_visible(), "boot_flags_white_nologs") != 0)
        return -1;
    if (expect(bootmode_progress_visible(), "boot_progress_flag") != 0)
        return -1;
    bootmode_set_logs_visible(1);
    bootmode_set_progress_visible(0);
    bootmode_set_theme(BOOT_THEME_DARK);
    if (expect(bootmode_theme() == BOOT_THEME_DARK && bootmode_logs_visible() && !bootmode_progress_visible(), "display_state_transition") != 0)
        return -1;
    unsigned char img[sizeof(exoimg_header_t) + 16];
    memset(img, 0, sizeof(img));
    memcpy(img, "EXOIMG1\0", 8);
    put32(img + 8, 2); put32(img + 12, 2); put32(img + 16, EXOIMG_FORMAT_RGBA8888); put32(img + 20, 16);
    exoimg_header_t h;
    if (expect(exoimg_validate(img, sizeof(img), &h) == 0 && h.width == 2 && h.height == 2, "exoimg_valid_header") != 0)
        return -1;
    img[0] = 'B';
    if (expect(exoimg_validate(img, sizeof(img), 0) != 0, "exoimg_reject_bad_magic") != 0)
        return -1;
    img[0] = 'E'; put32(img + 20, 12);
    if (expect(exoimg_validate(img, sizeof(img), 0) != 0, "exoimg_reject_bad_size") != 0)
        return -1;
    return 0;
}

static int test_fat32_fs(void) {
    static unsigned char image[16 * 512];
    char out[24];
    const char payload[] = "fat32-ok";

    make_test_fat32(image, sizeof(image));
    fs_mount(image, sizeof(image));
    if (expect(fs_is_fat32(), "fat32_mount") != 0)
        return -1;
    int fd = fs_open("/BOOT.TXT", 0x10 | 0x03 | 0x20);
    if (expect(fd >= 0, "fat32_open_create") != 0)
        return -1;
    if (expect(fs_write_fd(fd, payload, sizeof(payload)) == (long)sizeof(payload), "fat32_write") != 0)
        return -1;
    if (expect(fs_lseek_fd(fd, 0, 0) == 0, "fat32_lseek") != 0)
        return -1;
    memset(out, 0, sizeof(out));
    if (expect(fs_read_fd(fd, out, sizeof(payload)) == (long)sizeof(payload), "fat32_read") != 0)
        return -1;
    if (expect(memcmp(out, payload, sizeof(payload)) == 0, "fat32_read_data") != 0)
        return -1;
    if (expect(fs_file_size(fd) == (long)sizeof(payload), "fat32_file_size") != 0)
        return -1;
    if (expect(fs_close(fd) == 0, "fat32_close") != 0)
        return -1;
    return 0;
}

static int test_vfs(void) {
    char buf[32];
    vfs_stat_t st;
    vfs_dirent_t ents[4];

    if (expect(vfs_init() == 0, "vfs_init") != 0)
        return -1;
    if (expect(vfs_mkdir("/sys") == 0, "vfs_mkdir") != 0)
        return -1;
    int fd = vfs_open("/sys/backend.txt", VFS_O_CREAT | VFS_O_RDWR | VFS_O_TRUNC);
    if (expect(fd >= 0, "vfs_open_create") != 0)
        return -1;
    const char payload[] = "backend-ok";
    if (expect(vfs_write(fd, payload, sizeof(payload)) == (long)sizeof(payload), "vfs_write") != 0)
        return -1;
    if (expect(vfs_lseek(fd, 0, VFS_SEEK_SET) == 0, "vfs_lseek") != 0)
        return -1;
    memset(buf, 0, sizeof(buf));
    if (expect(vfs_read(fd, buf, sizeof(payload)) == (long)sizeof(payload), "vfs_read") != 0)
        return -1;
    if (expect(memcmp(buf, payload, sizeof(payload)) == 0, "vfs_read_data") != 0)
        return -1;
    if (expect(vfs_fstat(fd, &st) == 0 && st.type == VFS_TYPE_FILE && st.size == sizeof(payload), "vfs_fstat") != 0)
        return -1;
    if (expect(vfs_close(fd) == 0, "vfs_close") != 0)
        return -1;
    if (expect(vfs_rename("/sys/backend.txt", "/sys/renamed.txt") == 0, "vfs_rename") != 0)
        return -1;
    if (expect(vfs_stat("/sys/renamed.txt", &st) == 0 && st.type == VFS_TYPE_FILE, "vfs_stat") != 0)
        return -1;
    int dirfd = vfs_open("/sys", VFS_O_RDONLY);
    if (expect(dirfd >= 0, "vfs_open_dir") != 0)
        return -1;
    long dent_count = vfs_getdents(dirfd, ents, 4);
    if (expect(dent_count == 1 && strcmp(ents[0].name, "renamed.txt") == 0, "vfs_getdents") != 0)
        return -1;
    vfs_close(dirfd);
    if (expect(vfs_chdir("/sys") == 0, "vfs_chdir") != 0)
        return -1;
    memset(buf, 0, sizeof(buf));
    if (expect(vfs_getcwd(buf, sizeof(buf)) == 0 && strcmp(buf, "/sys") == 0, "vfs_getcwd") != 0)
        return -1;
    if (expect(vfs_chdir("/") == 0, "vfs_chdir_root") != 0)
        return -1;
    if (expect(vfs_unlink("/sys/renamed.txt") == 0, "vfs_unlink") != 0)
        return -1;
    return 0;
}

static int test_heap_growth(void) {
    mem_info_t before, after_fail;
    mem_get_info(&before);
    void *big = mem_alloc(before.heap_committed + 4096);
    mem_info_t after_grow;
    mem_get_info(&after_grow);
    if (expect(big != 0 && after_grow.grow_count > before.grow_count && after_grow.heap_committed > before.heap_committed, "heap_grows_past_committed") != 0)
        return -1;
    mem_free(big, before.heap_committed + 4096);
    void *too_big = mem_alloc(after_grow.heap_max + 4096);
    mem_get_info(&after_fail);
    if (expect(too_big == 0 && after_fail.alloc_fail_count > after_grow.alloc_fail_count, "heap_max_returns_null") != 0)
        return -1;
    return 0;
}

static int test_memctx(void) {
    memctx_stats_t stats;
    int ctx = memctx_create(100, 256);
    if (expect(ctx > 0, "memctx_create") != 0)
        return -1;
    void *a = memctx_alloc(ctx, 64);
    void *b = memctx_alloc(ctx, 128);
    if (expect(a != 0 && b != 0, "memctx_alloc") != 0)
        return -1;
    if (expect(memctx_stats(ctx, &stats) == 0 && stats.used == 192 && stats.peak == 192 && stats.allocations == 2, "memctx_stats") != 0)
        return -1;
    if (expect(memctx_alloc(ctx, 128) == 0, "memctx_quota") != 0)
        return -1;
    if (expect(memctx_free(ctx, a) == 0, "memctx_free") != 0)
        return -1;
    if (expect(memctx_stats(ctx, &stats) == 0 && stats.used == 128 && stats.allocations == 1, "memctx_stats_after_free") != 0)
        return -1;
    if (expect(memctx_destroy(ctx) == 0, "memctx_destroy") != 0)
        return -1;
    return 0;
}


static void make_test_elf(unsigned char *img, size_t bytes, uint16_t machine, uint32_t ph_type, uint64_t memsz) {
    memset(img, 0, bytes);
    elf_header_t *eh = (elf_header_t*)img;
    elf_phdr_t *ph = (elf_phdr_t*)(img + sizeof(elf_header_t));
    eh->e_magic = ELF_MAGIC;
    eh->e_class = ELF_CLASS_64;
    eh->e_data = ELF_DATA_LSB;
    eh->e_version = ELF_VERSION_CURRENT;
    eh->e_type = ELF_TYPE_EXEC;
    eh->e_machine = machine;
    eh->e_version2 = ELF_VERSION_CURRENT;
    eh->e_entry = 0x400100;
    eh->e_phoff = sizeof(elf_header_t);
    eh->e_ehsize = sizeof(elf_header_t);
    eh->e_phentsize = sizeof(elf_phdr_t);
    eh->e_phnum = 1;
    ph->p_type = ph_type;
    ph->p_flags = ELF_PF_R | ELF_PF_X;
    ph->p_offset = 0x100;
    ph->p_vaddr = 0x400100;
    ph->p_filesz = 4;
    ph->p_memsz = memsz;
    ph->p_align = 0x1000;
    img[0x100] = 0xC3;
    img[0x101] = 0x90;
    img[0x102] = 0x90;
    img[0x103] = 0x90;
}

static int test_embedded_initramfs_install(void) {
    vfs_stat_t st;
    if (expect(vfs_init() == 0, "initramfs_vfs_ready") != 0)
        return -1;
    const char bad[] = "x";
    if (expect(install_initramfs_file(NULL, bad, sizeof(bad)) != 0, "initramfs_reject_bad_write") != 0)
        return -1;
    if (expect(install_embedded_initramfs() == 0, "initramfs_install_all_files") != 0)
        return -1;
    if (expect(vfs_stat("/launchd.elf", &st) == 0 && st.type == VFS_TYPE_FILE && st.size > 0, "initramfs_launchd_exists") != 0)
        return -1;
    if (expect(vfs_stat("/launchd.cfg", &st) == 0 && st.type == VFS_TYPE_FILE && st.size > 0, "initramfs_launchd_cfg_exists") != 0)
        return -1;
    if (expect(vfs_stat("/shelld.elf", &st) == 0 && st.type == VFS_TYPE_FILE && st.size > 0, "initramfs_shelld_exists") != 0)
        return -1;
    int fd = vfs_open("/launchd.elf", VFS_O_RDONLY);
    unsigned char magic[4] = {0};
    long got = fd >= 0 ? vfs_read(fd, magic, sizeof(magic)) : -1;
    if (fd >= 0) vfs_close(fd);
    if (expect(got == 4 && magic[0] == 0x7f && magic[1] == 'E' && magic[2] == 'L' && magic[3] == 'F', "initramfs_launchd_keeps_elf_magic") != 0)
        return -1;
    return 0;
}

static int test_elf_loader(void) {
    unsigned char img[512];
    elf_image_t loaded;
    proc_info_t info;
    proc_init();
    int pid = proc_create("elf-test", 0);
    if (expect(pid == 1, "launchd_pid_is_1") != 0)
        return -1;

    make_test_elf(img, sizeof(img), ELF_MACHINE_X86_64, ELF_PT_LOAD, 16);
    if (expect(elf_validate(img, sizeof(img)) == 0, "elf_load_valid_launchd") != 0)
        return -1;
    if (expect(elf_load_process_image(pid, img, sizeof(img), &loaded) == 0, "launchd_entry_comes_from_elf") != 0)
        return -1;
    if (expect(((unsigned char*)loaded.load_base)[4] == 0 && ((unsigned char*)loaded.load_base)[15] == 0, "elf_zeroes_bss") != 0)
        return -1;
    if (expect(proc_attach_image(pid, "/launchd.elf", &loaded) == 0 && proc_info(pid, &info) == 0 && info.entry_point == (uintptr_t)loaded.entry, "launchd_is_not_kernel_fake_path") != 0)
        return -1;

    img[0] = 0;
    if (expect(elf_validate(img, sizeof(img)) != 0, "elf_reject_bad_magic") != 0)
        return -1;
    make_test_elf(img, sizeof(img), 3, ELF_PT_LOAD, 16);
    if (expect(elf_validate(img, sizeof(img)) != 0, "elf_reject_bad_machine") != 0)
        return -1;
    make_test_elf(img, sizeof(img), ELF_MACHINE_X86_64, ELF_PT_LOAD, 2);
    if (expect(elf_validate(img, sizeof(img)) != 0, "elf_reject_bad_program_header") != 0)
        return -1;
    return 0;
}

static int test_process_model_cleanup(void) {
    proc_info_t info;
    proc_init();
    int launchd = proc_create("launchd", 0);
    int shelld = proc_create("shelld", launchd);
    int helper = proc_create("helper", launchd);
    if (expect(launchd == 1 && shelld == 2 && helper == 3, "launchd_spawns_all_configured_daemons") != 0)
        return -1;
    if (expect(proc_info(shelld, &info) == 0 && info.parent_pid == launchd, "shelld_parent_is_launchd") != 0)
        return -1;
    if (expect(proc_set_current(shelld) == 0 && proc_current_pid() == 2 && proc_current_valid(), "shelld_runs_as_pid2_current") != 0)
        return -1;
    if (expect(proc_info(proc_current_pid(), &info) == 0 && info.parent_pid == 1, "shelld_current_parent_is_pid1") != 0)
        return -1;
    if (expect(proc_kill(1, -1) == PROC_ERR_PROTECTED && proc_info(1, &info) == 0 && info.state != PROC_STATE_EXITED, "kill_pid1_protected") != 0)
        return -1;
    if (expect(proc_kill(helper, -1) == 0 && proc_info(helper, &info) == 0 && info.state == PROC_STATE_EXITED, "kill_normal_child") != 0)
        return -1;
    if (expect(proc_kill(helper, -1) == PROC_ERR_INVALID_STATE, "kill_rejects_already_exited") != 0)
        return -1;
    int grandchild = proc_create("orphan-me", shelld);
    if (expect(grandchild == 4 && proc_kill(shelld, -1) == 0 && proc_current_pid() == 0 && !proc_current_valid(), "kill_current_shelld_clears_context") != 0)
        return -1;
    if (expect(proc_info(grandchild, &info) == 0 && info.parent_pid == 1, "orphan_reparent_to_pid1") != 0)
        return -1;
    int status = -1;
    if (expect(proc_wait(launchd, shelld, &status) == shelld && status == -1, "pid1_reaps_zombie") != 0)
        return -1;
    return 0;
}

static int test_proc(void) {
    proc_info_t info;
    int parent = proc_create("tester-parent", 0);
    if (expect(parent > 0, "proc_create_parent") != 0)
        return -1;
    if (expect(proc_set_current(parent) == 0 && proc_current_pid() == parent, "proc_set_current") != 0)
        return -1;
    int child = proc_create("tester-child", parent);
    if (expect(child > 0, "proc_create_child") != 0)
        return -1;
    void *block = proc_alloc(child, 96);
    if (expect(block != 0 && proc_free(child, block) == 0, "proc_memory") != 0)
        return -1;
    int fd = proc_open(child, "/procfile.txt", VFS_O_CREAT | VFS_O_RDWR | VFS_O_TRUNC);
    if (expect(fd >= 0, "proc_open") != 0)
        return -1;
    const char text[] = "proc-ok";
    char out[16];
    if (expect(proc_write(child, fd, text, sizeof(text)) == (long)sizeof(text), "proc_write") != 0)
        return -1;
    proc_lseek(child, fd, 0, VFS_SEEK_SET);
    memset(out, 0, sizeof(out));
    if (expect(proc_read(child, fd, out, sizeof(text)) == (long)sizeof(text) && memcmp(out, text, sizeof(text)) == 0, "proc_read") != 0)
        return -1;
    if (expect(proc_close(child, fd) == 0, "proc_close") != 0)
        return -1;
    if (expect(proc_info(child, &info) == 0 && info.parent_pid == parent, "proc_info") != 0)
        return -1;
    if (expect(proc_exit(child, 7) == 0, "proc_exit") != 0)
        return -1;
    int status = 0;
    if (expect(proc_wait(parent, child, &status) == child && status == 7, "proc_wait") != 0)
        return -1;
    return 0;
}

int backend_selftest_run(void) {
    test_log("[backend-test] starting backend driver tests\n");
    int failures = 0;
    int saved_logs_visible = bootmode_logs_visible();
    int saved_progress_visible = bootmode_progress_visible();
    uint32_t saved_theme = bootmode_theme();
    failures += test_boot_modes_and_exoimg() == 0 ? 0 : 1;
    bootmode_set_logs_visible(saved_logs_visible);
    bootmode_set_progress_visible(saved_progress_visible);
    bootmode_set_theme(saved_theme);
    failures += test_fat32_fs() == 0 ? 0 : 1;
    failures += test_vfs() == 0 ? 0 : 1;
    failures += test_embedded_initramfs_install() == 0 ? 0 : 1;
    proc_init();
    failures += test_elf_loader() == 0 ? 0 : 1;
    failures += test_process_model_cleanup() == 0 ? 0 : 1;
    proc_init();
    failures += test_memctx() == 0 ? 0 : 1;
    failures += test_heap_growth() == 0 ? 0 : 1;
    failures += test_proc() == 0 ? 0 : 1;
    if (failures == 0) {
        test_log("[backend-test] ALL BACKEND TESTS PASSED\n");
        return 0;
    }
    test_log("[backend-test] BACKEND TESTS FAILED\n");
    return -1;
}
