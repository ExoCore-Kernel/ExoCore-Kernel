#include "memctx.h"
#include "mem.h"
#include "memutils.h"

#define MEMCTX_MAX_CONTEXTS 32
#define MEMCTX_MAX_BLOCKS 128

typedef struct {
    void *ptr;
    size_t size;
    int ctx_id;
} memctx_block_t;

typedef struct {
    int active;
    int id;
    int owner_pid;
    memctx_stats_t stats;
} memctx_t;

static memctx_t contexts[MEMCTX_MAX_CONTEXTS];
static memctx_block_t blocks[MEMCTX_MAX_BLOCKS];
static int next_ctx_id = 1;

static memctx_t *find_ctx(int ctx_id) {
    for (int i = 0; i < MEMCTX_MAX_CONTEXTS; ++i) {
        if (contexts[i].active && contexts[i].id == ctx_id)
            return &contexts[i];
    }
    return 0;
}

static memctx_block_t *find_block(int ctx_id, void *ptr) {
    for (int i = 0; i < MEMCTX_MAX_BLOCKS; ++i) {
        if (blocks[i].ptr == ptr && blocks[i].ctx_id == ctx_id)
            return &blocks[i];
    }
    return 0;
}

void memctx_init(void) {
    memset(contexts, 0, sizeof(contexts));
    memset(blocks, 0, sizeof(blocks));
    next_ctx_id = 1;
}

int memctx_create(int owner_pid, size_t quota) {
    for (int i = 0; i < MEMCTX_MAX_CONTEXTS; ++i) {
        if (!contexts[i].active) {
            contexts[i].active = 1;
            contexts[i].id = next_ctx_id++;
            contexts[i].owner_pid = owner_pid;
            contexts[i].stats.quota = quota;
            contexts[i].stats.used = 0;
            contexts[i].stats.peak = 0;
            contexts[i].stats.allocations = 0;
            return contexts[i].id;
        }
    }
    return -1;
}

void *memctx_alloc(int ctx_id, size_t size) {
    if (size == 0)
        return 0;
    memctx_t *ctx = find_ctx(ctx_id);
    if (!ctx)
        return 0;
    if (ctx->stats.quota && ctx->stats.used + size > ctx->stats.quota)
        return 0;

    int slot = -1;
    for (int i = 0; i < MEMCTX_MAX_BLOCKS; ++i) {
        if (!blocks[i].ptr) {
            slot = i;
            break;
        }
    }
    if (slot < 0)
        return 0;

    void *ptr = mem_alloc(size);
    if (!ptr)
        return 0;

    blocks[slot].ptr = ptr;
    blocks[slot].size = size;
    blocks[slot].ctx_id = ctx_id;
    ctx->stats.used += size;
    if (ctx->stats.used > ctx->stats.peak)
        ctx->stats.peak = ctx->stats.used;
    ctx->stats.allocations++;
    return ptr;
}

int memctx_free(int ctx_id, void *ptr) {
    if (!ptr)
        return -1;
    memctx_t *ctx = find_ctx(ctx_id);
    memctx_block_t *blk = find_block(ctx_id, ptr);
    if (!ctx || !blk)
        return -1;
    mem_free(ptr, blk->size);
    if (ctx->stats.used >= blk->size)
        ctx->stats.used -= blk->size;
    if (ctx->stats.allocations)
        ctx->stats.allocations--;
    blk->ptr = 0;
    blk->size = 0;
    blk->ctx_id = 0;
    return 0;
}

int memctx_stats(int ctx_id, memctx_stats_t *stats) {
    memctx_t *ctx = find_ctx(ctx_id);
    if (!ctx || !stats)
        return -1;
    *stats = ctx->stats;
    return 0;
}

int memctx_destroy(int ctx_id) {
    memctx_t *ctx = find_ctx(ctx_id);
    if (!ctx)
        return -1;
    for (int i = 0; i < MEMCTX_MAX_BLOCKS; ++i) {
        if (blocks[i].ptr && blocks[i].ctx_id == ctx_id) {
            mem_free(blocks[i].ptr, blocks[i].size);
            blocks[i].ptr = 0;
            blocks[i].size = 0;
            blocks[i].ctx_id = 0;
        }
    }
    memset(ctx, 0, sizeof(*ctx));
    return 0;
}
