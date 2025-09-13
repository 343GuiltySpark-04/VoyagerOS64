/**
 * Copyright (c) 2025 Tristan Adams
 *
 * This software is released under the MIT License.
 * https://opensource.org/licenses/MIT
 */

// early_alloc.c
#include "include/early_alloc.h"
#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include "include/KernelUtils.h"

// --- Limine requests (defined in some other TU) ---
#include "include/limine.h"

#define PAGE_SIZE 4096ull
#define ALIGN_UP(x, a) (((x) + ((a) - 1)) & ~((a) - 1))
#define ALIGN_DOWN(x, a) ((x) & ~((a) - 1))

// Tiny local zero
static inline void memzero(void *p, size_t n)
{
    volatile uint8_t *d = (volatile uint8_t *)p;
    for (size_t i = 0; i < n; i++)
        d[i] = 0;
}

// --- Early bump state ---
static paddr_t g_base = 0;
static paddr_t g_cursor = 0;
static paddr_t g_end = 0;
static bool g_region_forced = false;

static inline bool memmap_ready(void)
{
    return memmap_req.response && memmap_req.response->entry_count > 0;
}

// Pick a USABLE region. Heuristics: prefer the largest USABLE below 4 GiB; fall back to largest anywhere.
static void pick_region_from_memmap(void)
{
    if (!memmap_ready())
        return;

    uint64_t best_size_below4g = 0, best_size_any = 0;
    paddr_t best_base_below4g = 0, best_end_below4g = 0;
    paddr_t best_base_any = 0, best_end_any = 0;

    for (size_t i = 0; i < memmap_req.response->entry_count; i++)
    {
        struct limine_memmap_entry *e = memmap_req.response->entries[i];
        if (e->type != LIMINE_MEMMAP_USABLE)
            continue;

        uint64_t start = ALIGN_UP(e->base, PAGE_SIZE);
        uint64_t end = ALIGN_DOWN(e->base + e->length, PAGE_SIZE);
        if (end <= start)
            continue;
        uint64_t size = end - start;

        if (end <= 0x100000000ull && size > best_size_below4g)
        {
            best_size_below4g = size;
            best_base_below4g = start;
            best_end_below4g = end;
        }
        if (size > best_size_any)
        {
            best_size_any = size;
            best_base_any = start;
            best_end_any = end;
        }
    }

    if (best_size_below4g)
    {
        g_base = best_base_below4g;
        g_end = best_end_below4g;
    }
    else
    {
        g_base = best_base_any;
        g_end = best_end_any;
    }
    g_cursor = g_base;
}

void early_set_region(paddr_t base, size_t page_count)
{
    g_base = base;
    g_cursor = base;
    g_end = base + page_count * PAGE_SIZE;
    g_region_forced = true;
}

void early_init(void)
{
    if (!g_region_forced)
    {
        pick_region_from_memmap();
    }
    // Minimal sanity: region must exist and be page aligned
    if (g_base == 0 || g_end <= g_base || (g_base & (PAGE_SIZE - 1)))
    {
        // Don't panic here; let the real allocator detect and panic if needed.
        // But it’s fair to be strict in a hobby kernel:
        extern void panic(const char *fmt, ...);
        panic("early_init: no suitable USABLE region found for early allocations");
    }
}

paddr_t early_alloc_page(void)
{
    if (g_cursor + PAGE_SIZE > g_end)
    {
        extern void panic(const char *fmt, ...);
        panic("early_alloc_page: out of early pages (used=%zu pages)", (size_t)((g_cursor - g_base) / PAGE_SIZE));
    }
    paddr_t p = g_cursor;
    g_cursor += PAGE_SIZE;
    return p;
}

void early_reserved_region(paddr_t *base, size_t *pages)
{
    if (base)
        *base = g_base;
    if (pages)
        *pages = (size_t)((g_cursor - g_base) / PAGE_SIZE);
}

size_t early_pages_used(void)
{
    return (size_t)((g_cursor - g_base) / PAGE_SIZE);
}
