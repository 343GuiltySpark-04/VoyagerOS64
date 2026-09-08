/**
 * Copyright (c) 2025 Tristan Adams
 *
 * This software is released under the MIT License.
 * https://opensource.org/licenses/MIT
 */

#include "../include/mm/kmalloc.h"
#include "../include/KernelUtils.h" // for k_mode.addr_debug? optional
#include "../include/lock.h"
#include "../include/paging/neo_framealloc.h" // frame_alloc
#include "../include/paging/paging.h" // PagingMapMemory, PagingUnmapMemory, PagingPhysicalMemory
#include "../include/printf.h"
#include <stdbool.h>
#include <stdint.h>

extern void     panic(const char *fmt, ...);
extern uint64_t ReadCR3(void);

#ifndef PAGING_FLAG_PRESENT
#define PAGING_FLAG_PRESENT (1ull << 0)
#endif
#ifndef PAGING_FLAG_WRITABLE
#define PAGING_FLAG_WRITABLE (1ull << 1)
#endif
#ifndef PAGING_FLAG_NO_EXECUTE
#define PAGING_FLAG_NO_EXECUTE (1ull << 63)
#endif

#define PAGE_SIZE 4096ull
#define HEAP_BASE 0xFFFF900000000000ull
#define HEAP_LIMIT (HEAP_BASE + (256ull * 1024 * 1024)) /* 256 MiB window */

#define ALIGN_UP(x, a) (((x) + ((a) -1)) & ~((a) -1))
#define ALIGN_DOWN(x, a) ((x) & ~((a) -1))
#define ALIGNMENT 16ull
#define MIN_SPLIT_SIZE 32ull /* minimal payload left after split (tune) */

typedef struct block_header
{
    size_t size_and_flags; // total block size (header+payload+footer); bit0 =
                           // allocated flag
    struct block_header *prev_free;
    struct block_header *next_free;
} block_t;

/* Footer mirrors size for O(1) backward coalescing */
typedef struct block_footer
{
    size_t size_and_flags;
} footer_t;

#define ALLOC_FLAG ((size_t) 1)
#define IS_ALLOC(h) ((h)->size_and_flags & ALLOC_FLAG)
#define SET_ALLOC(h) ((h)->size_and_flags |= ALLOC_FLAG)
#define CLR_ALLOC(h) ((h)->size_and_flags &= ~ALLOC_FLAG)
#define BLK_SIZE(h) ((h)->size_and_flags & ~ALLOC_FLAG)
#define SET_SIZE(h, sz)                                                    \
    do                                                                     \
    {                                                                      \
        (h)->size_and_flags = ((sz) | ((h)->size_and_flags & ALLOC_FLAG)); \
    } while (0)

/* Pad the header so payloads are 16-byte aligned. Keep every block size a
 * multiple of 16 so split blocks preserve that alignment. */
#define HDR_SIZE ALIGN_UP((size_t) sizeof(block_t), ALIGNMENT)
#define FTR_SIZE ((size_t) sizeof(footer_t))
#define OVERHEAD (HDR_SIZE + FTR_SIZE)
#define USABLE(h) (BLK_SIZE(h) - OVERHEAD)

static inline size_t total_size_for_payload(size_t payload)
{
    size_t aligned_payload = ALIGN_UP(payload, ALIGNMENT);
    return ALIGN_UP(aligned_payload + OVERHEAD, ALIGNMENT);
}

static inline footer_t *FOOTER_OF(block_t *h)
{
    return (footer_t *) ((uint8_t *) h + BLK_SIZE(h) - FTR_SIZE);
}
static inline block_t *NEXT_BLOCK(block_t *h)
{
    return (block_t *) ((uint8_t *) h + BLK_SIZE(h));
}
static inline block_t *PREV_BLOCK(block_t *h)
{
    footer_t *prev_ftr = (footer_t *) ((uint8_t *) h - FTR_SIZE);
    size_t    prev_sz  = prev_ftr->size_and_flags & ~ALLOC_FLAG;
    return (block_t *) ((uint8_t *) h - prev_sz);
}

/* Heap arena state */
static uint64_t   g_heap_curr = HEAP_BASE; // next unmapped VA
static spinlock_t g_heap_lock = SPINLOCK_INIT;
static block_t   *g_free_list =
    NULL; // circular or linear; we’ll keep it linear head

/* The legacy paging API expects a PHYSICAL PML4 address in its PageTable *
 * parameter and performs the HHDM translation internally. */
static inline struct PageTable *current_pml4_phys(void)
{
    uint64_t cr3 = ReadCR3() & ~0xfffull;
    return (struct PageTable *) (uintptr_t) cr3;
}

/* Free list ops */
static void fl_push(block_t *b)
{
    b->prev_free = NULL;
    b->next_free = g_free_list;
    if (g_free_list)
        g_free_list->prev_free = b;
    g_free_list = b;
}
static void fl_remove(block_t *b)
{
    if (b->prev_free)
        b->prev_free->next_free = b->next_free;
    else
        g_free_list = b->next_free;
    if (b->next_free)
        b->next_free->prev_free = b->prev_free;
    b->prev_free = b->next_free = NULL;
}

/* Map more pages and create a single big free block appended at g_heap_curr. */
static void heap_map_more(size_t bytes)
{
    size_t   need  = ALIGN_UP(bytes, PAGE_SIZE);
    uint64_t start = ALIGN_UP(g_heap_curr, PAGE_SIZE);
    uint64_t end   = start + need;
    if (end > HEAP_LIMIT)
        panic("kmalloc: VA heap exhausted");

    struct PageTable *p4 = current_pml4_phys();
    for (uint64_t va = start; va < end; va += PAGE_SIZE)
    {
        uint64_t phys = frame_alloc();
        PagingMapMemory(p4,
                        (void *) va,
                        (void *) phys,
                        PAGING_FLAG_PRESENT | PAGING_FLAG_WRITABLE |
                            PAGING_FLAG_NO_EXECUTE);
    }

    /* Make a new big free block over the newly mapped region */
    block_t *h        = (block_t *) (uintptr_t) start;
    size_t   blk_sz   = (size_t) (end - start);
    h->size_and_flags = blk_sz; // alloc bit off
    h->prev_free = h->next_free = NULL;

    footer_t *f       = FOOTER_OF(h);
    f->size_and_flags = blk_sz;

    fl_push(h);
    g_heap_curr = end;
}

/* First-fit search */
static block_t *fl_find_fit(size_t need_payload)
{
    size_t need_total = total_size_for_payload(need_payload);
    for (block_t *it = g_free_list; it; it = it->next_free)
    {
        if (BLK_SIZE(it) >= need_total)
            return it;
    }
    return NULL;
}

/* Split free block if large enough; return allocated header */
static block_t *allocate_from_block(block_t *h, size_t need_payload)
{
    size_t need_total = total_size_for_payload(need_payload);
    size_t old_sz     = BLK_SIZE(h);

    fl_remove(h);

    if (old_sz >= need_total + total_size_for_payload(MIN_SPLIT_SIZE))
    {
        /* Split: front part becomes allocated, tail becomes a free block */
        size_t alloc_sz = need_total;
        size_t tail_sz  = old_sz - alloc_sz;

        /* alloc part */
        SET_SIZE(h, alloc_sz);
        SET_ALLOC(h);
        FOOTER_OF(h)->size_and_flags = h->size_and_flags;

        /* tail free block */
        block_t *t        = (block_t *) ((uint8_t *) h + alloc_sz);
        t->size_and_flags = tail_sz; // free
        t->prev_free = t->next_free  = NULL;
        FOOTER_OF(t)->size_and_flags = tail_sz;

        fl_push(t);
        return h;
    }
    else
    {
        /* Take the whole block */
        SET_ALLOC(h);
        FOOTER_OF(h)->size_and_flags = h->size_and_flags;
        return h;
    }
}

/* Coalesce with neighbors if they are free */
static block_t *coalesce(block_t *h)
{
    /* Try next. g_heap_curr is the first currently-unmapped heap address. */
    block_t *n = NEXT_BLOCK(h);
    if ((uintptr_t) n < (uintptr_t) g_heap_curr)
    {
        if (!IS_ALLOC(n))
        {
            /* merge with next */
            fl_remove(n);
            size_t new_sz = BLK_SIZE(h) + BLK_SIZE(n);
            SET_SIZE(h, new_sz);
            FOOTER_OF(h)->size_and_flags = h->size_and_flags;
        }
    }
    /* Try prev: guard against base */
    if ((uintptr_t) h > (uintptr_t) HEAP_BASE)
    {
        footer_t *pf  = (footer_t *) ((uint8_t *) h - FTR_SIZE);
        size_t    psz = pf->size_and_flags & ~ALLOC_FLAG;
        block_t  *p   = (block_t *) ((uint8_t *) h - psz);
        if (!IS_ALLOC(p))
        {
            fl_remove(p);
            size_t new_sz = BLK_SIZE(p) + BLK_SIZE(h);
            SET_SIZE(p, new_sz);
            FOOTER_OF(p)->size_and_flags = p->size_and_flags;
            h                            = p;
        }
    }
    return h;
}

/* ========================= Public API ========================= */

void kmalloc_init(void)
{
    g_heap_curr = HEAP_BASE;
    g_free_list = NULL;
    /* Map an initial chunk to avoid early fragmentation; tune as you like. */
    heap_map_more(64 * PAGE_SIZE); // 256 KiB to start
}

void *kmalloc(size_t size)
{
    if (size == 0)
        return NULL;

    spinlock_acquire(&g_heap_lock);

    block_t *h = fl_find_fit(size);
    if (!h)
    {
        /* Map more and retry once */
        size_t grow = total_size_for_payload(size);
        if (grow < (128 * PAGE_SIZE))
            grow = 128 * PAGE_SIZE; // amortize mapping
        heap_map_more(grow);
        h = fl_find_fit(size);
        if (!h)
        {
            spinlock_release(&g_heap_lock);
            return NULL;
        } // still no memory
    }

    block_t *a       = allocate_from_block(h, size);
    void    *payload = (uint8_t *) a + HDR_SIZE;
    spinlock_release(&g_heap_lock);
    return payload;
}

void kfree(void *ptr)
{
    if (!ptr)
        return;
    spinlock_acquire(&g_heap_lock);

    block_t *h = (block_t *) ((uint8_t *) ptr - HDR_SIZE);
    if (!IS_ALLOC(h))
    {
        spinlock_release(&g_heap_lock);
        panic("kfree: double free %p", ptr);
    }
    CLR_ALLOC(h);
    FOOTER_OF(h)->size_and_flags = h->size_and_flags;

    /* Coalesce with neighbors and reinsert */
    block_t *c = coalesce(h);
    fl_push(c);

    spinlock_release(&g_heap_lock);
}

void *krealloc(void *ptr, size_t newsize)
{
    if (!ptr)
        return kmalloc(newsize);
    if (newsize == 0)
    {
        kfree(ptr);
        return NULL;
    }

    spinlock_acquire(&g_heap_lock);

    block_t *h   = (block_t *) ((uint8_t *) ptr - HDR_SIZE);
    size_t   cur = USABLE(h);
    if (cur >= newsize)
    {
        /* Optional: shrink-in-place if large extra -> split tail */
        spinlock_release(&g_heap_lock);
        return ptr;
    }

    /* Try to grow into next block if it is mapped, free and large enough. */
    block_t *n = NEXT_BLOCK(h);
    if ((uintptr_t) n < (uintptr_t) g_heap_curr && !IS_ALLOC(n))
    {
        size_t combined = cur + OVERHEAD + USABLE(n);
        if (combined >= newsize)
        {
            /* Merge in place */
            fl_remove(n);
            size_t new_sz = BLK_SIZE(h) + BLK_SIZE(n);
            SET_SIZE(h, new_sz);
            SET_ALLOC(h);
            FOOTER_OF(h)->size_and_flags = h->size_and_flags;
            // Optional: split tail if much larger than needed
            spinlock_release(&g_heap_lock);
            return ptr;
        }
    }

    /* Fallback: allocate new, copy, free old */
    spinlock_release(&g_heap_lock);
    void *np = kmalloc(newsize);
    if (!np)
        return NULL;
    // memcpy (safe length = cur)
    extern void *memcpy(void *, const void *, size_t);
    memcpy(np, ptr, cur);
    kfree(ptr);
    return np;
}

size_t kmalloc_usable_size(void *ptr)
{
    if (!ptr)
        return 0;
    block_t *h = (block_t *) ((uint8_t *) ptr - HDR_SIZE);
    return USABLE(h);
}

/* Optional: crude dump */
void kmalloc_dump(void)
{
#if 0
    printf_("[kmalloc] free list:\n");
    for (block_t* it = g_free_list; it; it = it->next_free) {
        printf_("  free blk %p size=%llu\n", it, (unsigned long long)BLK_SIZE(it));
    }
#endif
}
