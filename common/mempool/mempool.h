#ifndef CLIBS_MEMPOOL_H
#define CLIBS_MEMPOOL_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

/* Fixed-block allocator over caller-provided storage. No heap. A free-list
 * threads through the free blocks (the next-pointer lives in each free block,
 * so the stride is forced to >= sizeof(void*) and aligned). Fields are private
 * by convention — use the methods, never poke fields.
 * Not reentrant: guard with ATOMIC_BLOCK/cli if shared with an ISR. */
typedef struct {
    uint8_t *buf;          /* private: base of caller storage */
    size_t   block_size;   /* private: effective stride (aligned up) */
    size_t   block_count;  /* private: total blocks */
    size_t   free_count;   /* private: blocks currently free */
    void    *free_list;    /* private: free-list head */
} mempool;

#define MEMPOOL_ALIGN      (sizeof(void *))
/* Per-block stride: block_size rounded up to MEMPOOL_ALIGN (>= sizeof(void*)). */
#define MEMPOOL_STRIDE(bs) ((((bs) + (MEMPOOL_ALIGN - 1)) / MEMPOOL_ALIGN) * MEMPOOL_ALIGN)
/* Bytes the caller's storage must provide for `n` blocks of `bs` bytes each. */
#define MEMPOOL_BYTES(bs, n) (MEMPOOL_STRIDE(bs) * (n))

/* Constructor. `storage` must be MEMPOOL_ALIGN-aligned and at least
 * MEMPOOL_BYTES(block_size, block_count) bytes. Threads every block onto the
 * free list. NULL self -> no-op. */
void   mempool_init(mempool *self, void *storage, size_t block_size, size_t block_count);
/* Allocate one block. Returns NULL when the pool is exhausted (or NULL self). */
void  *mempool_alloc(mempool *self);
/* Return a block to the pool. NULL blk -> no-op; NULL self -> no-op.
 * Pointer-belongs-to-pool validation and double-free detection are the
 * caller's responsibility (out of scope, kept minimal). */
void   mempool_free(mempool *self, void *blk);
/* Number of blocks currently free. NULL self -> 0. */
size_t mempool_free_count(const mempool *self);
/* Total number of blocks the pool holds. NULL self -> 0. */
size_t mempool_capacity(const mempool *self);

#endif /* CLIBS_MEMPOOL_H */
