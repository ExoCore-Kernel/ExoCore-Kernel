#include "vfs.h"
#include "mem.h"
#include "memutils.h"

#define VFS_MAX_NODES 64
#define VFS_MAX_OPEN 32

typedef struct {
    int used;
    uint32_t inode;
    uint32_t type;
    int parent;
    char name[VFS_MAX_NAME + 1];
    unsigned char *data;
    size_t size;
    size_t capacity;
} vfs_node_t;

typedef struct {
    int used;
    int node;
    size_t offset;
    int flags;
} vfs_open_t;

static vfs_node_t nodes[VFS_MAX_NODES];
static vfs_open_t open_files[VFS_MAX_OPEN];
static uint32_t next_inode = 1;
static int cwd_node = 0;
static int ready = 0;

static size_t copy_name(char *dst, const char *src, size_t len) {
    size_t n = 0;
    while (n < len && n < VFS_MAX_NAME && src[n]) {
        dst[n] = src[n];
        ++n;
    }
    dst[n] = '\0';
    return n;
}

static int name_eq(const char *name, const char *seg, size_t len) {
    size_t i = 0;
    while (i < len && name[i] && name[i] == seg[i])
        ++i;
    return i == len && name[i] == '\0';
}

static int alloc_node(uint32_t type, int parent, const char *name, size_t len) {
    for (int i = 0; i < VFS_MAX_NODES; ++i) {
        if (!nodes[i].used) {
            memset(&nodes[i], 0, sizeof(nodes[i]));
            nodes[i].used = 1;
            nodes[i].inode = next_inode++;
            nodes[i].type = type;
            nodes[i].parent = parent;
            copy_name(nodes[i].name, name, len);
            return i;
        }
    }
    return -1;
}

static int find_child(int parent, const char *seg, size_t len) {
    if (parent < 0 || parent >= VFS_MAX_NODES || !nodes[parent].used || nodes[parent].type != VFS_TYPE_DIR)
        return -1;
    for (int i = 0; i < VFS_MAX_NODES; ++i) {
        if (nodes[i].used && nodes[i].parent == parent && name_eq(nodes[i].name, seg, len))
            return i;
    }
    return -1;
}

static int child_count(int parent) {
    int count = 0;
    for (int i = 0; i < VFS_MAX_NODES; ++i) {
        if (i != parent && nodes[i].used && nodes[i].parent == parent)
            ++count;
    }
    return count;
}

static const char *next_segment(const char *p, const char **seg, size_t *len) {
    while (*p == '/')
        ++p;
    *seg = p;
    while (*p && *p != '/')
        ++p;
    *len = (size_t)(p - *seg);
    return p;
}

static int resolve_path(const char *path) {
    if (!ready || !path || !*path)
        return -1;
    int node = (path[0] == '/') ? 0 : cwd_node;
    const char *p = path;
    const char *seg;
    size_t len;

    while (*(p = next_segment(p, &seg, &len))) {
        if (len == 0)
            continue;
        if (len == 1 && seg[0] == '.')
            continue;
        if (len == 2 && seg[0] == '.' && seg[1] == '.') {
            if (node != 0)
                node = nodes[node].parent;
            continue;
        }
        node = find_child(node, seg, len);
        if (node < 0)
            return -1;
    }
    if (len) {
        if (len == 1 && seg[0] == '.')
            return node;
        if (len == 2 && seg[0] == '.' && seg[1] == '.')
            return node == 0 ? 0 : nodes[node].parent;
        node = find_child(node, seg, len);
    }
    return node;
}

static int resolve_parent(const char *path, int *parent, const char **leaf, size_t *leaf_len) {
    if (!path || !*path || !parent || !leaf || !leaf_len)
        return -1;
    int node = (path[0] == '/') ? 0 : cwd_node;
    const char *p = path;
    const char *seg = 0;
    size_t len = 0;
    const char *last = 0;
    size_t last_len = 0;

    while (*(p = next_segment(p, &seg, &len))) {
        if (len == 0 || (len == 1 && seg[0] == '.'))
            continue;
        if (last) {
            if (last_len == 2 && last[0] == '.' && last[1] == '.') {
                if (node != 0)
                    node = nodes[node].parent;
            } else {
                node = find_child(node, last, last_len);
                if (node < 0 || nodes[node].type != VFS_TYPE_DIR)
                    return -1;
            }
        }
        last = seg;
        last_len = len;
    }
    if (len) {
        if (last) {
            if (last_len == 2 && last[0] == '.' && last[1] == '.') {
                if (node != 0)
                    node = nodes[node].parent;
            } else {
                node = find_child(node, last, last_len);
                if (node < 0 || nodes[node].type != VFS_TYPE_DIR)
                    return -1;
            }
        }
        last = seg;
        last_len = len;
    }
    if (!last || last_len == 0 || (last_len == 1 && last[0] == '.') || (last_len == 2 && last[0] == '.' && last[1] == '.'))
        return -1;
    *parent = node;
    *leaf = last;
    *leaf_len = last_len;
    return 0;
}

static int ensure_capacity(vfs_node_t *node, size_t need) {
    if (need <= node->capacity)
        return 0;
    size_t cap = node->capacity ? node->capacity : 64;
    while (cap < need)
        cap *= 2;
    unsigned char *new_data = mem_alloc(cap);
    if (!new_data)
        return -1;
    if (node->data && node->size)
        memcpy(new_data, node->data, node->size);
    if (node->data)
        mem_free(node->data, node->capacity);
    node->data = new_data;
    node->capacity = cap;
    return 0;
}

static int fill_stat(int node, vfs_stat_t *st) {
    if (node < 0 || node >= VFS_MAX_NODES || !nodes[node].used || !st)
        return -1;
    st->type = nodes[node].type;
    st->size = nodes[node].size;
    st->inode = nodes[node].inode;
    st->children = (nodes[node].type == VFS_TYPE_DIR) ? (uint32_t)child_count(node) : 0;
    return 0;
}

int vfs_init(void) {
    memset(nodes, 0, sizeof(nodes));
    memset(open_files, 0, sizeof(open_files));
    next_inode = 1;
    cwd_node = 0;
    ready = 1;
    int root = alloc_node(VFS_TYPE_DIR, 0, "", 0);
    return root == 0 ? 0 : -1;
}

int vfs_is_ready(void) {
    return ready;
}

int vfs_mkdir(const char *path) {
    int parent;
    const char *leaf;
    size_t len;
    if (resolve_parent(path, &parent, &leaf, &len) != 0)
        return -1;
    if (find_child(parent, leaf, len) >= 0)
        return -1;
    return alloc_node(VFS_TYPE_DIR, parent, leaf, len) >= 0 ? 0 : -1;
}

int vfs_unlink(const char *path) {
    int node = resolve_path(path);
    if (node <= 0 || nodes[node].type == VFS_TYPE_DIR || child_count(node) != 0)
        return -1;
    if (nodes[node].data)
        mem_free(nodes[node].data, nodes[node].capacity);
    memset(&nodes[node], 0, sizeof(nodes[node]));
    return 0;
}

int vfs_rename(const char *old_path, const char *new_path) {
    int node = resolve_path(old_path);
    int parent;
    const char *leaf;
    size_t len;
    if (node <= 0 || resolve_parent(new_path, &parent, &leaf, &len) != 0)
        return -1;
    if (find_child(parent, leaf, len) >= 0)
        return -1;
    nodes[node].parent = parent;
    copy_name(nodes[node].name, leaf, len);
    return 0;
}

int vfs_chdir(const char *path) {
    int node = resolve_path(path);
    if (node < 0 || nodes[node].type != VFS_TYPE_DIR)
        return -1;
    cwd_node = node;
    return 0;
}

static int build_path(int node, char *buf, size_t len) {
    if (!buf || len == 0)
        return -1;
    if (node == 0) {
        if (len < 2)
            return -1;
        buf[0] = '/';
        buf[1] = '\0';
        return 0;
    }
    char tmp[VFS_MAX_PATH];
    tmp[0] = '\0';
    int cur = node;
    while (cur != 0) {
        char next[VFS_MAX_PATH];
        size_t name_len = strlen(nodes[cur].name);
        size_t tmp_len = strlen(tmp);
        if (name_len + tmp_len + 2 >= sizeof(next))
            return -1;
        next[0] = '/';
        memcpy(next + 1, nodes[cur].name, name_len);
        memcpy(next + 1 + name_len, tmp, tmp_len + 1);
        memcpy(tmp, next, strlen(next) + 1);
        cur = nodes[cur].parent;
    }
    if (strlen(tmp) + 1 > len)
        return -1;
    memcpy(buf, tmp, strlen(tmp) + 1);
    return 0;
}

int vfs_getcwd(char *buf, size_t len) {
    return build_path(cwd_node, buf, len);
}

int vfs_open(const char *path, int flags) {
    int node = resolve_path(path);
    if (node < 0 && (flags & VFS_O_CREAT)) {
        int parent;
        const char *leaf;
        size_t len;
        if (resolve_parent(path, &parent, &leaf, &len) != 0)
            return -1;
        node = alloc_node(VFS_TYPE_FILE, parent, leaf, len);
    }
    if (node < 0)
        return -1;
    if (nodes[node].type == VFS_TYPE_DIR && (flags & (VFS_O_WRONLY | VFS_O_CREAT | VFS_O_TRUNC | VFS_O_APPEND)))
        return -1;
    if ((flags & VFS_O_TRUNC) && nodes[node].type == VFS_TYPE_FILE && ensure_capacity(&nodes[node], 0) == 0)
        nodes[node].size = 0;
    for (int fd = 0; fd < VFS_MAX_OPEN; ++fd) {
        if (!open_files[fd].used) {
            open_files[fd].used = 1;
            open_files[fd].node = node;
            open_files[fd].flags = flags;
            open_files[fd].offset = (flags & VFS_O_APPEND) ? nodes[node].size : 0;
            return fd;
        }
    }
    return -1;
}

long vfs_read(int fd, void *buf, size_t len) {
    if (fd < 0 || fd >= VFS_MAX_OPEN || !open_files[fd].used || !buf)
        return -1;
    vfs_node_t *node = &nodes[open_files[fd].node];
    if (node->type != VFS_TYPE_FILE || !(open_files[fd].flags & VFS_O_RDONLY))
        return -1;
    if (open_files[fd].offset >= node->size)
        return 0;
    if (open_files[fd].offset + len > node->size)
        len = node->size - open_files[fd].offset;
    memcpy(buf, node->data + open_files[fd].offset, len);
    open_files[fd].offset += len;
    return (long)len;
}

long vfs_write(int fd, const void *buf, size_t len) {
    if (fd < 0 || fd >= VFS_MAX_OPEN || !open_files[fd].used || !buf)
        return -1;
    vfs_node_t *node = &nodes[open_files[fd].node];
    if (node->type != VFS_TYPE_FILE || !(open_files[fd].flags & VFS_O_WRONLY))
        return -1;
    if ((open_files[fd].flags & VFS_O_APPEND))
        open_files[fd].offset = node->size;
    size_t end = open_files[fd].offset + len;
    if (ensure_capacity(node, end) != 0)
        return -1;
    memcpy(node->data + open_files[fd].offset, buf, len);
    open_files[fd].offset = end;
    if (end > node->size)
        node->size = end;
    return (long)len;
}

long vfs_lseek(int fd, long offset, int whence) {
    if (fd < 0 || fd >= VFS_MAX_OPEN || !open_files[fd].used)
        return -1;
    vfs_node_t *node = &nodes[open_files[fd].node];
    long base;
    if (whence == VFS_SEEK_SET)
        base = 0;
    else if (whence == VFS_SEEK_CUR)
        base = (long)open_files[fd].offset;
    else if (whence == VFS_SEEK_END)
        base = (long)node->size;
    else
        return -1;
    long next = base + offset;
    if (next < 0)
        return -1;
    open_files[fd].offset = (size_t)next;
    return next;
}

int vfs_close(int fd) {
    if (fd < 0 || fd >= VFS_MAX_OPEN || !open_files[fd].used)
        return -1;
    memset(&open_files[fd], 0, sizeof(open_files[fd]));
    return 0;
}

int vfs_stat(const char *path, vfs_stat_t *st) {
    return fill_stat(resolve_path(path), st);
}

int vfs_fstat(int fd, vfs_stat_t *st) {
    if (fd < 0 || fd >= VFS_MAX_OPEN || !open_files[fd].used)
        return -1;
    return fill_stat(open_files[fd].node, st);
}

long vfs_getdents(int fd, vfs_dirent_t *ents, size_t max_ents) {
    if (fd < 0 || fd >= VFS_MAX_OPEN || !open_files[fd].used || !ents)
        return -1;
    int dir = open_files[fd].node;
    if (nodes[dir].type != VFS_TYPE_DIR)
        return -1;
    size_t emitted = 0;
    size_t index = 0;
    for (int i = 0; i < VFS_MAX_NODES && emitted < max_ents; ++i) {
        if (i == dir || !nodes[i].used || nodes[i].parent != dir)
            continue;
        if (index++ < open_files[fd].offset)
            continue;
        ents[emitted].type = nodes[i].type;
        ents[emitted].inode = nodes[i].inode;
        ents[emitted].size = nodes[i].size;
        copy_name(ents[emitted].name, nodes[i].name, strlen(nodes[i].name));
        ++emitted;
    }
    open_files[fd].offset += emitted;
    return (long)emitted;
}
