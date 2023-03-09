/* Leonard Kevin McGuire Jr (kmcg3413@gmail.com) (www.kmcg3413.net) */

#include <stdint.h>
#include "include/heap.h"

/**
 * @brief Initialize a heap. This is called at the start of a program to initialize the heap.
 * @param * heap
 * @return Returns nothing ; kheapBMInit () does nothing
 */
void k_heapBMInit(KHEAPBM *heap)
{
    heap->fblock = 0;
}

/**
 * @brief Adds a block to the heap.
 * @param * heap
 * @param addr The address of the block to add.
 * @param size The size of the block in bytes.
 * @param bsize The size of the block in bytes.
 * @return 0 on success - 1 on failure
 */
int k_heapBMAddBlock(KHEAPBM *heap, uintptr_t addr, uint64_t size, uint64_t bsize)
{
    KHEAPBLOCKBM *b;
    uintptr_t bmsz;
    uint8_t *bm;

    b = (KHEAPBLOCKBM *)addr;
    bmsz = k_heapBMGetBMSize(size, bsize);
    bm = (uint8_t *)(addr + sizeof(KHEAPBLOCKBM));
    /* important to set isBMInside... (last argument) */
    return k_heapBMAddBlockEx(heap, addr + sizeof(KHEAPBLOCKBM), size - sizeof(KHEAPBLOCKBM), bsize, b, bm, 1);
}

/**
 * @brief Get BM size of heap
 * @param size Size of heap in bytes
 * @param bsize Size of block allocation in bytes
 * @return Size of memory in bytes
 */
uintptr_t k_heapBMGetBMSize(uintptr_t size, uint64_t bsize)
{
    return size / bsize;
}

/**
 * @brief Add a block to the heap.
 * @param * heap
 * @param addr The address of the block to add.
 * @param size The size of the block in bytes.
 * @param bsize The size of the block in bytes.
 * @param * b
 * @param * bm
 * @param isBMInside If true the block is in the bitmap.
 * @return 0 on success - 1 on failure
 */
int k_heapBMAddBlockEx(KHEAPBM *heap, uintptr_t addr, uint64_t size, uint64_t bsize, KHEAPBLOCKBM *b, uint8_t *bm, uint8_t isBMInside)
{
    b->size = size;
    b->bsize = bsize;
    b->data = addr;
    b->bm = bm;

    b->next = heap->fblock;
    heap->fblock = b;

    // Clear the bitmap.
    for (uint64_t x = 0; x < size / bsize; ++x)
    {
        bm[x] = 0;
    }

    // Calculate the number of blocks needed for the bitmap.
    uint64_t bcnt = (size / bsize) * bsize < size ? (size / bsize) + 1 : (size / bsize);

    // If BM is not inside leave this space available.
    if (isBMInside)
    {
        // Reserve room for bitmap.
        for (uint64_t x = 0; x < bcnt; ++x)
        {
            bm[x] = 5; // 5 is used to indicate a reserved block.
        }
    }

    // Set the last free block index to the total number of blocks minus one.
    b->lfb = bcnt - 1;

    // Set the number of used blocks to the total number of blocks.
    b->used = bcnt;

    return 1; // Return 1 as success indicator.
}

/**
 * @brief Get NID of node a and node b
 * @param a Node to look for in BM
 * @param b Node to look for in A
 * @return NID of node a and
 */
static uint8_t k_heapBMGetNID(uint8_t a, uint8_t b)
{
    uint8_t c;
    for (c = a + 1; c == b || c == 0; ++c)
        ;
    return c;
}

/**
 * @brief Allocate memory for a block of memory.
 * @param * heap
 * @param size The size of the memory block.
 * @return Pointer to the memory block
 */
void *k_heapBMAlloc(KHEAPBM *heap, uint64_t size)
{
    return k_heapBMAllocBound(heap, size, 0);
}

/**
 * @brief Allocate space in a heap.
 * @param * heap
 * @param size The size of the memory to allocate.
 * @param bound The upper bound ( exclusive ) of the allocation.
 * @return Pointer to the memory allocated
 */
void *k_heapBMAllocBound(KHEAPBM *heap, uint64_t size, uint64_t bound)
{
    KHEAPBLOCKBM *b;
    uint8_t *bm;
    uint64_t bcnt;
    uint64_t x, y, z;
    uint64_t bneed;
    uint8_t nid;
    uint64_t max;

    bound = ~(~0 << bound);
    /* iterate blocks */
    for (b = heap->fblock; b; b = b->next)
    {
        /* check if block has enough room */
        if (b->size - (b->used * b->bsize) >= size)
        {
            bcnt = b->size / b->bsize;
            bneed = (size / b->bsize) * b->bsize < size ? size / b->bsize + 1 : size / b->bsize;
            bm = (uint8_t *)b->bm;

            for (x = (b->lfb + 1 >= bcnt ? 0 : b->lfb + 1); x != b->lfb; ++x)
            {
                /* just wrap around */
                if (x >= bcnt)
                {
                    x = 0;
                }

                /*
                    this is used to allocate on specified boundaries larger than the block size
                */
                if ((((x * b->bsize) + b->data) & bound) != 0)
                    continue;

                if (bm[x] == 0)
                {
                    /* count free blocks */
                    max = bcnt - x;
                    for (y = 0; bm[x + y] == 0 && y < bneed && y < max; ++y)
                        ;

                    /* we have enough, now allocate them */
                    if (y == bneed)
                    {
                        /* find ID that does not match left or right */
                        nid = k_heapBMGetNID(bm[x - 1], bm[x + y]);

                        /* allocate by setting id */
                        for (z = 0; z < y; ++z)
                        {
                            bm[x + z] = nid;
                        }
                        /* optimization */
                        b->lfb = (x + bneed) - 2;

                        /* count used blocks NOT bytes */
                        b->used += y;
                        return (void *)((x * b->bsize) + b->data);
                    }

                    /* x will be incremented by one ONCE more in our FOR loop */
                    x += (y - 1);
                    continue;
                }
            }
        }
    }

    return 0;
}

/**
 * @brief Set a value in the heap.
 * @param * heap
 * @param ptr The address of the data to set.
 * @param size The size of the data to set.
 * @param rval The value to set
 */
void k_heapBMSet(KHEAPBM *heap, uintptr_t ptr, uintptr_t size, uint8_t rval)
{
    KHEAPBLOCKBM *b;
    uintptr_t ptroff, endoff;
    uint64_t bi, x, ei;
    uint8_t *bm;
    uint8_t id;
    uint64_t max;

    for (b = heap->fblock; b; b = b->next)
    {
        /* check if region effects block */
        if (
            /* head end resides inside block */
            (ptr >= b->data && ptr < b->data + b->size) ||
            /* tail end resides inside block */
            ((ptr + size) >= b->data && (ptr + size) < b->data + b->size) ||
            /* spans across but does not start or end in block */
            (ptr < b->data && (ptr + size) > b->data + b->size))
        {
            /* found block */
            if (ptr >= b->data)
            {
                ptroff = ptr - b->data; /* get offset to get block */
                /* block offset in BM */
                bi = ptroff / b->bsize;
            }
            else
            {
                /* do not start negative on bitmap */
                bi = 0;
            }

            /* access bitmap pointer in local variable */
            bm = b->bm;

            ptr = ptr + size;
            endoff = ptr - b->data;

            /* end index inside bitmap */
            ei = (endoff / b->bsize) * b->bsize < endoff ? (endoff / b->bsize) + 1 : endoff / b->bsize;
            ++ei;

            /* region could span past end of a block so adjust */
            max = b->size / b->bsize;
            max = ei > max ? max : ei;

            /* set bitmap buckets */
            for (x = bi; x < max; ++x)
            {
                bm[x] = rval;
            }

            /* update free block count */
            if (rval == 0)
            {
                b->used -= ei - bi;
            }
            else
            {
                b->used += ei - bi;
            }

            /* do not return as region could span multiple blocks.. so check the rest */
        }
    }

    /* this error needs to be raised or reported somehow */
    return;
}

/**
 * @brief Free memory allocated by k_heapBMAlloc.
 * @param * heap
 * @param * ptr
 */
void k_heapBMFree(KHEAPBM *heap, void *ptr)
{
    KHEAPBLOCKBM *b;
    uintptr_t ptroff;
    uint64_t bi, x;
    uint8_t *bm;
    uint8_t id;
    uint64_t max;

    for (b = heap->fblock; b; b = b->next)
    {
        if ((uintptr_t)ptr > b->data && (uintptr_t)ptr < b->data + b->size)
        {
            /* found block */
            ptroff = (uintptr_t)ptr - b->data; /* get offset to get block */
            /* block offset in BM */
            bi = ptroff / b->bsize;
            /* .. */
            bm = b->bm;
            /* clear allocation */
            id = bm[bi];

            max = b->size / b->bsize;
            for (x = bi; bm[x] == id && x < max; ++x)
            {
                bm[x] = 0;
            }
            /* update free block count */
            b->used -= x - bi;
            return;
        }
    }

    /* this error needs to be raised or reported somehow */
    return;
}