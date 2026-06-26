#include "mempool.h"

void mempool_init(mempool *self, void *storage, size_t block_size, size_t block_count)
{
    if (!self) {
        return;
    }

    size_t stride = MEMPOOL_STRIDE(block_size);

    self->buf = (uint8_t *)storage;
    self->block_size = stride;
    self->block_count = block_count;
    self->free_count = block_count;
    self->free_list = NULL;

    /* Push every block onto the free list; the next-pointer lives in the block. */
    for (size_t i = 0; i < block_count; i++) {
        void *blk = self->buf + i * stride;
        *(void **)blk = self->free_list;
        self->free_list = blk;
    }
}

void *mempool_alloc(mempool *self)
{
    if (!self || !self->free_list) {
        return NULL;
    }

    void *blk = self->free_list;
    self->free_list = *(void **)blk;
    self->free_count--;
    return blk;
}

void mempool_free(mempool *self, void *blk)
{
    if (!self || !blk) {
        return;
    }

    *(void **)blk = self->free_list;
    self->free_list = blk;
    self->free_count++;
}

size_t mempool_free_count(const mempool *self)
{
    return self ? self->free_count : 0;
}

size_t mempool_capacity(const mempool *self)
{
    return self ? self->block_count : 0;
}
