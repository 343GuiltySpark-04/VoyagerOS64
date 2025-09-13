/**
 * Copyright (c) 2025 Tristan Adams
 *
 * This software is released under the MIT License.
 * https://opensource.org/licenses/MIT
 */

// framealloc.c
#include "include/paging/neo_framealloc.h"
#include "include/lock.h"
#include "include/limine.h"
#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

extern struct limine_memmap_request memmap_request;
extern struct limine_module_request module_request;
extern volatile struct limine_kernel_address_request kernel_address_request;

#define PAGE_SIZE 4096ull
#define ALIGN_UP(x, a) (((x) + ((a) - 1)) & ~((a) - 1))
#define ALIGN_DOWN(x, a) ((x) & ~((a) - 1))

static inline void memzero(void *p, size_t n)
{
    volatile uint8_t *d = (volatile uint8_t *)p;
    for (size_t i = 0; i < n; i++)
        d[i] = 0;
}
static inline void memset8(void *p, uint8_t v, size_t n)
{
    volatile uint8_t *d = (volatile uint8_t *)p;
    for (size_t i = 0; i < n; i++)
        d[i] = v;
}
static inline void panicf(const char *fmt, ...)
{
    extern void panic(const char *fmt, ...);
    va_list ap;
    va_start(ap, fmt); // We don't forward va_list cleanly here; delegate.
    panic(fmt);        // Keep simple: your panic likely ignores extra args anyway.
    va_end(ap);
}

// --- Global state ---
static uint8_t *g_alloc_bm = 0;  // 1=allocated/reserved, 0=free
static uint8_t *g_usable_bm = 0; // 1=USABLE, 0=not usable
static size_t g_frame_count = 0;
static uint64_t g_max_phys = 0;
static size_t g_free_count = 0;
static size_t g_hint = 0; // search hint
static spinlock_t memlock_t = SPINLOCK_INIT;

// --- Bit ops ---
static inline void bm_set(uint8_t *bm, size_t i) { bm[i >> 3] |= (uint8_t)(1u << (i & 7)); }
static inline void bm_clear(uint8_t *bm, size_t i) { bm[i >> 3] &= (uint8_t)~(1u << (i & 7)); }
static inline bool bm_test(const uint8_t *bm, size_t i) { return (bm[i >> 3] >> (i & 7)) & 1u; }

// Index/addr helpers
static inline size_t frame_index_of(paddr_t p) { return (size_t)(p / PAGE_SIZE); }
static inline paddr_t frame_addr_of(size_t i) { return (paddr_t)(i * PAGE_SIZE); }

static void mark_range_allocated(paddr_t base, paddr_t end)
{
    if (end <= base)
        return;
    size_t i0 = frame_index_of(ALIGN_DOWN(base, PAGE_SIZE));
    size_t i1 = frame_index_of(ALIGN_UP(end, PAGE_SIZE));
    if (i1 > g_frame_count)
        i1 = g_frame_count;
    for (size_t i = i0; i < i1; i++)
    {
        if (!bm_test(g_alloc_bm, i))
        {
            bm_set(g_alloc_bm, i);
            if (g_free_count)
                g_free_count--;
        }
    }
}

static void mark_range_usable_and_free(paddr_t base, paddr_t end)
{
    if (end <= base)
        return;
    size_t i0 = frame_index_of(ALIGN_UP(base, PAGE_SIZE));
    size_t i1 = frame_index_of(ALIGN_DOWN(end, PAGE_SIZE));
    if (i1 <= i0)
        return;
    if (i1 > g_frame_count)
        i1 = g_frame_count;

    for (size_t i = i0; i < i1; i++)
    {
        if (!bm_test(g_usable_bm, i))
            bm_set(g_usable_bm, i); // mark usable
        if (bm_test(g_alloc_bm, i))
        { // make free if was allocated-by-default
            bm_clear(g_alloc_bm, i);
            g_free_count++;
        }
    }
}

static void compute_max_phys_and_count(void)
{
    g_max_phys = 0;
    if (!memmap_request.response)
    {
        extern void panic(const char *fmt, ...);
        panic("frame_init: Limine memmap missing");
    }
    for (size_t i = 0; i < memmap_request.response->entry_count; i++)
    {
        struct limine_memmap_entry *e = memmap_request.response->entries[i];
        uint64_t end = e->base + e->length;
        if (end > g_max_phys)
            g_max_phys = end;
    }
    g_frame_count = (size_t)ALIGN_UP(g_max_phys, PAGE_SIZE) / PAGE_SIZE;
    if (g_frame_count == 0)
    {
        extern void panic(const char *fmt, ...);
        panic("frame_init: zero frames?");
    }
}

void frame_init(void)
{
    // 1) Derive frame_count/max_phys
    compute_max_phys_and_count();

    // 2) Allocate bitmaps from early allocator
    size_t bm_bytes = ALIGN_UP((g_frame_count + 7) / 8, PAGE_SIZE);
    paddr_t alloc_bm_phys = early_alloc_page(); // consume at least 1 page
    size_t alloc_bm_pages = 1;
    while (alloc_bm_pages * PAGE_SIZE < bm_bytes)
    {
        early_alloc_page();
        alloc_bm_pages++;
    }
    // Map them virtually somehow? If you have HHDM, convert. Otherwise, identity map early.
    // For allocator bookkeeping we just need a pointer; assume an HHDM direct map:
    extern struct limine_hhdm_request hhdm_request;
    if (!hhdm_request.response)
    {
        extern void panic(const char *fmt, ...);
        panic("frame_init: HHDM required for bitmap access");
    }
    uint64_t hhdm = hhdm_request.response->offset;
    g_alloc_bm = (uint8_t *)(alloc_bm_phys + hhdm);
    g_usable_bm = (uint8_t *)(alloc_bm_phys + hhdm + (bm_bytes + PAGE_SIZE - 1) / PAGE_SIZE * PAGE_SIZE / 2); // <- we'll place second bitmap right after first

    // Simpler: allocate both bitmaps separately for clarity
    // If you prefer clarity, comment out the above two lines and use:
    // {
    //   paddr_t a = early_alloc_page();
    //   size_t need = bm_bytes - PAGE_SIZE;
    //   while (need > 0) { early_alloc_page(); need -= PAGE_SIZE; }
    //   g_alloc_bm = (uint8_t*)(a + hhdm);
    //   paddr_t b = early_alloc_page();
    //   need = bm_bytes - PAGE_SIZE;
    //   while (need > 0) { early_alloc_page(); need -= PAGE_SIZE; }
    //   g_usable_bm = (uint8_t*)(b + hhdm);
    // }

    // For deterministic layout without gymnastics, do the simple separate-allocation path:
    {
        // Re-do cleanly
        // allocate alloc_bm
        size_t need = bm_bytes;
        paddr_t a = early_alloc_page();
        need -= PAGE_SIZE;
        while (need > 0)
        {
            early_alloc_page();
            need -= PAGE_SIZE;
        }
        g_alloc_bm = (uint8_t *)(a + hhdm);

        // allocate usable_bm
        need = bm_bytes;
        paddr_t b = early_alloc_page();
        need -= PAGE_SIZE;
        while (need > 0)
        {
            early_alloc_page();
            need -= PAGE_SIZE;
        }
        g_usable_bm = (uint8_t *)(b + hhdm);
    }

    // 3) Initialize bitmaps
    memset8(g_alloc_bm, 0xFF, bm_bytes); // everything allocated/reserved by default
    memzero(g_usable_bm, bm_bytes);      // nothing usable yet
    g_free_count = 0;

    // 4) Mark USABLE ranges as usable+free
    for (size_t i = 0; i < memmap_request.response->entry_count; i++)
    {
        struct limine_memmap_entry *e = memmap_request.response->entries[i];
        if (e->type == LIMINE_MEMMAP_USABLE)
        {
            mark_range_usable_and_free(e->base, e->base + e->length);
        }
    }

    // 5) Re-reserve kernel
    // Option A: Limine kernel address response
    if (!kernel_address_request.response)
    {
        extern void panic(const char *fmt, ...);
        panic("frame_init: kernel address response missing");
    }
    uint64_t kphys_base = kernel_address_request.response->physical_base;
    uint64_t kvirt_base = kernel_address_request.response->virtual_base;
    (void)kvirt_base; // unused here
    // You need kernel size; easiest is linker symbols:
    extern char __kernel_start_phys[], __kernel_end_phys[];
    uint64_t kphys_end = (uint64_t)__kernel_end_phys;
    uint64_t kphys_begin = (uint64_t)__kernel_start_phys;
    // If you don't have those, derive size from your own metadata.

    mark_range_allocated(kphys_begin, kphys_end);

    // 6) Reserve modules (phys addresses provided by Limine)
    if (module_request.response)
    {
        for (size_t i = 0; i < module_request.response->module_count; i++)
        {
            struct limine_file *m = module_request.response->modules[i];
            mark_range_allocated(m->address, m->address + m->size);
        }
    }

    // 7) Reserve early allocator consumption
    paddr_t early_base;
    size_t early_pages;
    early_reserved_region(&early_base, &early_pages);
    mark_range_allocated(early_base, early_base + early_pages * PAGE_SIZE);

    // Done. We can now allocate/free frames concurrently.
}

// --- Public API ---
paddr_t frame_alloc(void)
{
    spinlock_acquire(&memlock_t);
    if (g_free_count == 0)
    {
        spinlock_release(&memlock_t);
        extern void panic(const char *fmt, ...);
        panic("frame_alloc: out of physical memory");
    }

    size_t start = g_hint;
    for (size_t pass = 0; pass < 2; pass++)
    {
        size_t i = (pass == 0) ? start : 0;
        size_t end = (pass == 0) ? g_frame_count : start;
        for (; i < end; i++)
        {
            if (bm_test(g_usable_bm, i) && !bm_test(g_alloc_bm, i))
            {
                bm_set(g_alloc_bm, i);
                g_free_count--;
                g_hint = i + 1;
                paddr_t p = frame_addr_of(i);
                spinlock_release(&memlock_t);
                return p;
            }
        }
    }

    spinlock_release(&memlock_t);
    extern void panic(const char *fmt, ...);
    panic("frame_alloc: free count nonzero but no free frame found (bitmap corruption?)");
}

void frame_free(paddr_t paddr)
{
    if ((paddr & (PAGE_SIZE - 1)) != 0)
    {
        extern void panic(const char *fmt, ...);
        panic("frame_free: unaligned paddr=0x%llx", (unsigned long long)paddr);
    }
    if (paddr >= g_max_phys)
    {
        extern void panic(const char *fmt, ...);
        panic("frame_free: out-of-bounds paddr=0x%llx", (unsigned long long)paddr);
    }

    size_t i = frame_index_of(paddr);
    spinlock_acquire(&memlock_t);

    if (!bm_test(g_usable_bm, i))
    {
        spinlock_release(&memlock_t);
        extern void panic(const char *fmt, ...);
        panic("frame_free: frame 0x%llx not in USABLE memory", (unsigned long long)paddr);
    }
    if (!bm_test(g_alloc_bm, i))
    {
        spinlock_release(&memlock_t);
        extern void panic(const char *fmt, ...);
        panic("frame_free: double-free frame 0x%llx", (unsigned long long)paddr);
    }

    bm_clear(g_alloc_bm, i);
    g_free_count++;
    if (i < g_hint)
        g_hint = i; // opportunistic hint improvement
    spinlock_release(&memlock_t);
}

bool frame_is_allocated(paddr_t paddr)
{
    if ((paddr & (PAGE_SIZE - 1)) != 0 || paddr >= g_max_phys)
        return true; // treat invalid as allocated
    size_t i = frame_index_of(paddr);
    return bm_test(g_alloc_bm, i);
}

size_t frame_total_count(void) { return g_frame_count; }
size_t frame_free_count(void) { return g_free_count; }
