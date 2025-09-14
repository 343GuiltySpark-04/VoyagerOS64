/**
 * Copyright (c) 2025 Tristan Adams
 *
 * This software is released under the MIT License.
 * https://opensource.org/licenses/MIT
 */

#include "../include/lock.h" // spinlock
#include "../include/mm/kheap.h"
#include "../include/paging/frame_supplier.h" // PagingGetFreeFrame not needed here, but paging.h uses it
#include "../include/paging/neo_framealloc.h" // frame_alloc, frame_free
#include "../include/paging/paging.h" // PagingMapMemory, PagingUnmapMemory, PagingPhysicalMemory
#include "../include/printf.h" // optional debug
#include <stddef.h>
#include <stdint.h>

/* Your panic */
extern void panic(const char *fmt, ...);

/* From your paging.c (export declarations) */
uint64_t ReadCR3(void);

/* We’ll translate cr3 phys→virt through your “high half” helper. */
extern uint64_t TranslateToHighHalfMemoryAddress(uint64_t physicalAddress);

/* Paging flags you already defined */
#ifndef PAGING_FLAG_PRESENT
#define PAGING_FLAG_PRESENT (1ull << 0)
#endif
#ifndef PAGING_FLAG_WRITABLE
#define PAGING_FLAG_WRITABLE (1ull << 1)
#endif
#ifndef PAGING_FLAG_USER_ACCESSIBLE
#define PAGING_FLAG_USER_ACCESSIBLE (1ull << 2)
#endif
#ifndef PAGING_FLAG_NO_EXECUTE
#define PAGING_FLAG_NO_EXECUTE (1ull << 63)
#endif

/* =========================
   Kernel heap VA window
   ========================= */
#define KHEAP_BASE 0xFFFF900000000000ull
#define KHEAP_LIMIT (KHEAP_BASE + (256ull * 1024 * 1024)) /* 256 MiB */
#define PAGE_SIZE 4096ull

/* Simple linear VA cursor; liballoc carves sub-blocks from mapped spans. */
static uint64_t g_heap_cursor = KHEAP_BASE;

/* One global lock used by the liballoc hooks. */
static spinlock_t g_heap_lock = SPINLOCK_INIT;

/* Helpers */
static inline uint64_t align_up(uint64_t x, uint64_t a)
{
    return (x + (a - 1)) & ~(a - 1);
}

/* Current PML4 virtual pointer */
static inline struct PageTable *current_pml4(void)
{
    uint64_t cr3 = ReadCR3() & ~0xfffull; // phys address
    return (struct PageTable *) TranslateToHighHalfMemoryAddress(cr3);
}

/* === kheap API ===== */
void kheap_init(void)
{
    g_heap_cursor = KHEAP_BASE;
}

void kheap_reserve_pages(size_t pages)
{
    struct PageTable *p4    = current_pml4();
    uint64_t          start = align_up(g_heap_cursor, PAGE_SIZE);
    uint64_t          end   = start + pages * PAGE_SIZE;

    if (end > KHEAP_LIMIT)
        panic("kheap_reserve_pages: out of VA heap");

    for (uint64_t va = start; va < end; va += PAGE_SIZE)
    {
        uint64_t phys = frame_alloc();
        PagingMapMemory(p4,
                        (void *) va,
                        (void *) phys,
                        PAGING_FLAG_PRESENT | PAGING_FLAG_WRITABLE |
                            PAGING_FLAG_NO_EXECUTE);
    }
    g_heap_cursor = end;
}

/* =========================
   liballoc required hooks
   ========================= */

/* Lock: return 0 on success. */
int neo_liballoc_lock(void)
{
    spinlock_acquire(&g_heap_lock);
    return 0;
}

/* Unlock: return 0 on success. */
int neo_liballoc_unlock(void)
{
    spinlock_release(&g_heap_lock);
    return 0;
}

/* Allocate 'pages' pages and return a contiguous VA span. */
void *neo_liballoc_alloc(int pages)
{
    if (pages <= 0)
        return 0;

    struct PageTable *p4 = current_pml4();

    /* Pick next VA chunk */
    uint64_t start = align_up(g_heap_cursor, PAGE_SIZE);
    uint64_t end   = start + (uint64_t) pages * PAGE_SIZE;
    if (end > KHEAP_LIMIT)
        return 0;

    /* Map pages one-by-one */
    for (uint64_t va = start; va < end; va += PAGE_SIZE)
    {
        uint64_t phys = frame_alloc();
        PagingMapMemory(p4,
                        (void *) va,
                        (void *) phys,
                        PAGING_FLAG_PRESENT | PAGING_FLAG_WRITABLE |
                            PAGING_FLAG_NO_EXECUTE);
    }

    g_heap_cursor = end;
    return (void *) start;
}

/* Free 'pages' pages starting at 'ptr' */
int neo_liballoc_free(void *ptr, int pages)
{
    if (ptr == 0 || pages <= 0)
        return -1;

    struct PageTable *p4 = current_pml4();

    uint64_t va = (uint64_t) ptr;
    for (int i = 0; i < pages; i++, va += PAGE_SIZE)
    {
        /* Find backing phys, unmap, return frame */
        void *phys_ptr = PagingPhysicalMemory(p4, (void *) va);
        if (!phys_ptr)
        {
            /* Already unmapped? treat as error to be strict */
            return -1;
        }
        uint64_t phys = (uint64_t) phys_ptr;
        PagingUnmapMemory(p4, (void *) va);
        frame_free(phys);
    }
    return 0;
}
