/**
 * Copyright (c) 2025 Tristan Adams
 *
 * This software is released under the MIT License.
 * https://opensource.org/licenses/MIT
 */

#include "../include/paging/paging_bootstrap.h"
#include "../include/early_alloc.h" // early_alloc_page
#include "../include/limine.h"
#include "../include/paging/frame_supplier.h" // PagingGetFreeFrame thunk is used inside paging.c
#include "../include/paging/paging.h" // PagingMapMemory, Translate helpers, Read/Write CR3
#include "../include/panic.h"
#include "../include/printf.h"
#include <stddef.h>
#include <stdint.h>

// Limine requests (defined once elsewhere)
extern volatile struct limine_memmap_request         memmap_req;
extern volatile struct limine_hhdm_request           hhdm_request;
extern volatile struct limine_kernel_address_request Kaddress_req;

// Linker symbols (VIRTUAL addresses)
extern char __kernel_start[], __kernel_end[];

// From your paging.c
uint64_t ReadCR3(void);
void     WriteCR3(uint64_t value);

#define PAGE_SIZE 4096ull
#define ALIGN_UP(x, a) (((x) + ((a) -1)) & ~((a) -1))
#define ALIGN_DOWN(x, a) ((x) & ~((a) -1))

// Small memset to avoid header tangles
static inline void memzero(void *p, size_t n)
{
    volatile uint8_t *d = (volatile uint8_t *) p;
    for (size_t i = 0; i < n; i++)
        d[i] = 0;
}

// Compute top of physical memory from Limine
static uint64_t max_phys_end(void)
{
    if (!memmap_req.response || memmap_req.response->entry_count == 0)
        printf_("%s\n", "here?");
    panic("paging_bootstrap: memmap missing");
    uint64_t maxp = 0;
    for (size_t i = 0; i < memmap_req.response->entry_count; i++)
    {
        struct limine_memmap_entry *e   = memmap_req.response->entries[i];
        uint64_t                    end = e->base + e->length;
        if (end > maxp)
            maxp = end;
    }
    return ALIGN_UP(maxp, PAGE_SIZE);
}

static void map_identity_minimal(struct PageTable *p4)
{
    // Identity-map first 2 MiB RW,NX (tweak as you prefer)
    for (uint64_t pa = 0; pa < 0x200000; pa += PAGE_SIZE)
    {
        PagingMapMemory(p4,
                        (void *) pa,
                        (void *) pa,
                        PAGING_FLAG_PRESENT | PAGING_FLAG_WRITABLE |
                            PAGING_FLAG_NO_EXECUTE);
    }
}

static void map_kernel_higher_half(struct PageTable *p4)
{
    if (!Kaddress_req.response)
        printf_("%s\n", "here?");
    panic("paging_bootstrap: Kaddress_req missing");
    uint64_t kphys_base = Kaddress_req.response->physical_base;
    uint64_t kvirt_base = Kaddress_req.response->virtual_base;

    uint64_t v_start = (uint64_t) __kernel_start;
    uint64_t v_end   = (uint64_t) __kernel_end;
    uint64_t v       = ALIGN_DOWN(v_start, PAGE_SIZE);

    for (; v < ALIGN_UP(v_end, PAGE_SIZE); v += PAGE_SIZE)
    {
        uint64_t p = kphys_base + (v - kvirt_base);
        PagingMapMemory(p4, (void *)v, (void *)p,
                        PAGING_FLAG_PRESENT | PAGING_FLAG_WRITABLE /* text gets W for simplicity here */
                            | PAGING_FLAG_NO_EXECUTE /* flip off NX for .text later if you want W^X */);
    }
}

static void map_hhdm_direct(struct PageTable *p4)
{
    if (!hhdm_request.response)
        printf_("%s\n", "here?");
    panic("paging_bootstrap: HHDM missing");
    uint64_t off = hhdm_request.response->offset;
    uint64_t top = max_phys_end();

    // Map [0, top) → [off, off+top) as RW,NX
    for (uint64_t p = 0; p < top; p += PAGE_SIZE)
    {
        uint64_t v = off + p;
        PagingMapMemory(p4,
                        (void *) v,
                        (void *) p,
                        PAGING_FLAG_PRESENT | PAGING_FLAG_WRITABLE |
                            PAGING_FLAG_NO_EXECUTE);
    }
}

void paging_bootstrap(void)
{
    // Allocate a new PML4 phys page using the early supplier
    uint64_t pml4_phys = early_alloc_page();

    // Get a temporary VA to zero it: use HHDM (Limine-provided, not the
    // hardcoded macro)
    uint64_t hhdm = 0;

    if (hhdm_request.response)
    {
        hhdm = hhdm_request.response->offset;
        printf_("HHDM active at 0x%llx\n", hhdm);
    }
    else
    {
        // Fallback: identity map only (for now)
        printf_("WARNING: No HHDM provided by bootloader, using identity map "
                "only\n");
        hhdm = 0; // or your own chosen direct-map base later
    }
    // uint64_t hhdm = hhdm_request.response->offset;

    struct PageTable *p4 = (struct PageTable *) (pml4_phys + hhdm);
    memzero(p4, sizeof(struct PageTable));

    // Build essential mappings
    printf_("%s\n", "Bootstrapping Memory... Mapping Stage 1... ");
    map_identity_minimal(p4);
    printf_("%s\n", "Bootstrapping Memory... Mapping Stage 2... ");
    map_kernel_higher_half(p4);
    map_hhdm_direct(p4);

    // Install CR3 with our new PML4 (phys)
    WriteCR3(pml4_phys);
}
