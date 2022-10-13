#include <stdint.h>
#include "include/memUtils.h"

/* Leonard Kevin McGuire Jr (kmcg3413@gmail.com) (www.kmcg3413.net) */

void k_heapBMInit(KHEAPBM *heap)
{
    heap->fblock = 0;
}

int k_heapBMAddBlock(KHEAPBM *heap, uintptr_t addr, uint32_t size, uint32_t bsize)
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

uintptr_t k_heapBMGetBMSize(uintptr_t size, uint32_t bsize)
{
    return size / bsize;
}

int k_heapBMAddBlockEx(KHEAPBM *heap, uintptr_t addr, uint32_t size, uint32_t bsize, KHEAPBLOCKBM *b, uint8_t *bm, uint8_t isBMInside)
{
    uint32_t bcnt;
    uint32_t x;

    b->size = size;
    b->bsize = bsize;
    b->data = addr;
    b->bm = bm;

    b->next = heap->fblock;
    heap->fblock = b;

    bcnt = size / bsize;

    /* clear bitmap */
    for (x = 0; x < bcnt; ++x)
    {
        bm[x] = 0;
    }

    bcnt = (bcnt / bsize) * bsize < bcnt ? bcnt / bsize + 1 : bcnt / bsize;

    /* if BM is not inside leave this space avalible */
    if (isBMInside)
    {
        /* reserve room for bitmap */
        for (x = 0; x < bcnt; ++x)
        {
            bm[x] = 5;
        }
    }

    b->lfb = bcnt - 1;

    b->used = bcnt;

    return 1;
}

static uint8_t k_heapBMGetNID(uint8_t a, uint8_t b)
{
    uint8_t c;
    for (c = a + 1; c == b || c == 0; ++c)
        ;
    return c;
}

void *k_heapBMAlloc(KHEAPBM *heap, uint32_t size)
{
    return k_heapBMAllocBound(heap, size, 0);
}

void *k_heapBMAllocBound(KHEAPBM *heap, uint32_t size, uint32_t bound)
{
    KHEAPBLOCKBM *b;
    uint8_t *bm;
    uint32_t bcnt;
    uint32_t x, y, z;
    uint32_t bneed;
    uint8_t nid;
    uint32_t max;

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

void k_heapBMSet(KHEAPBM *heap, uintptr_t ptr, uintptr_t size, uint8_t rval)
{
    KHEAPBLOCKBM *b;
    uintptr_t ptroff, endoff;
    uint32_t bi, x, ei;
    uint8_t *bm;
    uint8_t id;
    uint32_t max;

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

void k_heapBMFree(KHEAPBM *heap, void *ptr)
{
    KHEAPBLOCKBM *b;
    uintptr_t ptroff;
    uint32_t bi, x;
    uint8_t *bm;
    uint8_t id;
    uint32_t max;

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