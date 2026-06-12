#include "backend_test.h"
#include "console.h"
#include "serial.h"
#include "vfs.h"
#include "proc.h"
#include "memctx.h"
#include "memutils.h"

static void test_log(const char *msg) {
    console_puts(msg);
    serial_write(msg);
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
    failures += test_vfs() == 0 ? 0 : 1;
    proc_init();
    failures += test_memctx() == 0 ? 0 : 1;
    failures += test_proc() == 0 ? 0 : 1;
    if (failures == 0) {
        test_log("[backend-test] ALL BACKEND TESTS PASSED\n");
        return 0;
    }
    test_log("[backend-test] BACKEND TESTS FAILED\n");
    return -1;
}
