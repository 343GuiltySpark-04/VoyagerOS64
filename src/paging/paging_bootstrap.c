/**
 * Copyright (c) 2025 Tristan Adams
 *
 * This software is released under the MIT License.
 * https://opensource.org/licenses/MIT
 */

#include "../include/early_alloc.h"
#include "../include/limine.h"
#include "../include/paging/paging_bootstrap.h"
#include "../include/printf.h"
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

/* ------------------------------------------------------------------ */
/* Limine requests (DEFINED ONCE in your limine_requests.c)           */
/* ------------------------------------------------------------------ */
extern volatile struct limine_memmap_request         memmap_req;
extern volatile struct limine_hhdm_request           hhdm_request;
extern volatile struct limine_kernel_address_request Kaddress_req;

/* Linker symbols: VIRTUAL kernel bounds */
extern char __kernel_start[], __kernel_end[];
extern char __text_start[], __text_end[];
extern char __rodata_start[], __rodata_end[];
extern char __data_start[], __bss_end[];

/* Your panic */
extern void panic(const char *fmt, ...);

/* Tiny memset to avoid old headers */
static inline void memzero(void *p, size_t n)
{
    volatile uint8_t *d = (volatile uint8_t *) p;
    for (size_t i = 0; i < n; i++)
        d[i] = 0;
}

/* CR3 helpers */
extern uint64_t readCR3();
extern void     writeCR3(uint64_t arg);
extern void     breakpoint();

/* ------------------------------------------------------------------ */
/* x86_64 paging bits / helpers                                       */
/* ------------------------------------------------------------------ */
#define PTE_P (1ull << 0)   /* Present */
#define PTE_W (1ull << 1)   /* Writable */
#define PTE_U (1ull << 2)   /* User (unused here) */
#define PTE_PS (1ull << 7)  /* Page Size (2MiB on PD level) */
#define PTE_NX (1ull << 63) /* No-Execute */

#define PAGE_SIZE 4096ull
#define PAGE_2M_SIZE (2ull * 1024 * 1024)
#define ENTRIES_PER_PT 512ull
#define ADDR_MASK 0x000FFFFFFFFFF000ull
#define ADDR_MASK_2M 0x000FFFFFFFFFE000ull

#define ALIGN_UP(x, a) (((x) + ((a) -1)) & ~((a) -1))
#define ALIGN_DOWN(x, a) ((x) & ~((a) -1))

struct __attribute__((aligned(0x1000))) page_table
{
    uint64_t e[512];
};

/* Global PML4 we build */
static uint64_t           g_pml4_phys = 0;
static struct page_table *g_pml4_virt = 0;
static uint64_t           g_hhdm      = 0;

/* Accessors */
uint64_t paging_pml4_phys(void)
{
    return g_pml4_phys;
}
void *paging_pml4_virt(void)
{
    return (void *) g_pml4_virt;
}

static void map_range_4k(struct page_table *pml4,
                         uint64_t           va,
                         uint64_t           pa,
                         uint64_t           len,
                         uint64_t           flags)
{
    uint64_t end = va + len;
    for (; va < end; va += PAGE_SIZE, pa += PAGE_SIZE)
    {
        map_4k(pml4, va, pa, flags);
    }
}

/* phys -> virt via HHDM (0 means "no HHDM", avoid using in that case) */
static inline void *phys_to_virt(uint64_t phys)
{
    return (void *) (phys + g_hhdm);
}

/* Allocate a zeroed page table */
static struct page_table *alloc_table(uint64_t *out_phys)
{
    uint64_t phys = early_alloc_page();
    if (g_hhdm == 0)
        panic("paging_bootstrap: HHDM required for table zeroing (g_hhdm==0)");
    struct page_table *pt = (struct page_table *) phys_to_virt(phys);
    memzero(pt, sizeof(*pt));
    if (out_phys)
        *out_phys = phys;
    return pt;
}

/* Split VA into PML4/PDPT/PD/PT indices */
static inline void vaddr_indices(uint64_t v, uint64_t idx[4])
{
    idx[0] = (v >> 39) & 0x1FF; /* PML4 i */
    idx[1] = (v >> 30) & 0x1FF; /* PDPT i */
    idx[2] = (v >> 21) & 0x1FF; /* PD   i */
    idx[3] = (v >> 12) & 0x1FF; /* PT   i */
}

static inline uint64_t
virt_to_phys_kernel(uint64_t vaddr, uint64_t kphys_base, uint64_t kvirt_base)
{
    return kphys_base + (vaddr - kvirt_base);
}

/* Create next-level table if absent; return its virt pointer */
static struct page_table *
get_or_make_next(struct page_table *cur, uint64_t idx, uint64_t *phys_out)
{
    uint64_t e = cur->e[idx];
    if (e & PTE_P)
    {
        uint64_t child_phys = e & ADDR_MASK;
        if (phys_out)
            *phys_out = child_phys;
        return (struct page_table *) phys_to_virt(child_phys);
    }
    uint64_t           child_phys = 0;
    struct page_table *child      = alloc_table(&child_phys);
    cur->e[idx]                   = (child_phys & ADDR_MASK) | PTE_P | PTE_W;
    if (phys_out)
        *phys_out = child_phys;
    return child;
}

/* Ensure a PD exists (we’ll set a 2MiB entry there) */
static struct page_table *get_or_make_pd(struct page_table *pml4,
                                         uint64_t           pml4i,
                                         uint64_t           pdpti,
                                         uint64_t          *pd_phys_out)
{
    uint64_t           junk;
    struct page_table *pdpt = get_or_make_next(pml4, pml4i, &junk);
    struct page_table *pd   = get_or_make_next(pdpt, pdpti, pd_phys_out);
    return pd;
}

/* Map a single 4KiB page */
static void
map_4k(struct page_table *pml4, uint64_t va, uint64_t pa, uint64_t flags)
{
    uint64_t idx[4];
    vaddr_indices(va, idx);

    uint64_t           junk;
    struct page_table *pdpt = get_or_make_next(pml4, idx[0], &junk);
    struct page_table *pd   = get_or_make_next(pdpt, idx[1], &junk);
    struct page_table *pt   = get_or_make_next(pd, idx[2], &junk);

    pt->e[idx[3]] = (pa & ADDR_MASK) | (flags & ~ADDR_MASK);
}

/* Map a single 2MiB page (PD-level with PS bit) */
static void
map_2m(struct page_table *pml4, uint64_t va, uint64_t pa, uint64_t flags)
{
    uint64_t idx[4];
    vaddr_indices(va, idx);
    uint64_t           pd_phys;
    struct page_table *pd = get_or_make_pd(pml4, idx[0], idx[1], &pd_phys);
    pd->e[idx[2]] =
        (pa & ADDR_MASK_2M) | (flags & ~ADDR_MASK) | PTE_P | PTE_W | PTE_PS;
}

/* Map [va, pa, len) with 2MiB chunks where possible, 4KiB at edges */
static void map_range_huge_first(struct page_table *pml4,
                                 uint64_t           va,
                                 uint64_t           pa,
                                 uint64_t           len,
                                 uint64_t           base_flags)
{
    uint64_t end = pa + len;

    /* Head: 4KiB until both VA & PA are 2MiB-aligned */
    while (pa < end && (((pa | va) & (PAGE_2M_SIZE - 1)) != 0))
    {
        map_4k(pml4, va, pa, base_flags | PTE_P | PTE_W | PTE_NX);
        pa += PAGE_SIZE;
        va += PAGE_SIZE;
    }

    /* Middle: 2MiB chunks */
    while (pa + PAGE_2M_SIZE <= end)
    {
        map_2m(pml4, va, pa, base_flags | PTE_NX);
        pa += PAGE_2M_SIZE;
        va += PAGE_2M_SIZE;
    }

    /* Tail: 4KiB remainder */
    while (pa < end)
    {
        map_4k(pml4, va, pa, base_flags | PTE_P | PTE_W | PTE_NX);
        pa += PAGE_SIZE;
        va += PAGE_SIZE;
    }
}

/* Minimal identity map for sanity */
static void map_identity_minimal(struct page_table *pml4)
{
    for (uint64_t p = 0; p < 0x200000; p += PAGE_SIZE)
    {
        map_4k(pml4, p, p, PTE_P | PTE_W | PTE_NX);
    }
}

/* Kernel higher-half mapping using Limine kernel bases + linker symbols */
static void map_kernel_higher_half(struct page_table *pml4)
{
    if (!Kaddress_req.response)
        panic("paging_bootstrap: kernel address request missing");

    uint64_t kphys_base = Kaddress_req.response->physical_base;
    uint64_t kvirt_base = Kaddress_req.response->virtual_base;

    /* Align each section to page boundaries */
    uint64_t t0 = ALIGN_DOWN((uint64_t) __text_start, PAGE_SIZE);
    uint64_t t1 = ALIGN_UP((uint64_t) __text_end, PAGE_SIZE);
    uint64_t r0 = ALIGN_DOWN((uint64_t) __rodata_start, PAGE_SIZE);
    uint64_t r1 = ALIGN_UP((uint64_t) __rodata_end, PAGE_SIZE);
    uint64_t d0 = ALIGN_DOWN((uint64_t) __data_start, PAGE_SIZE);
    uint64_t d1 = ALIGN_UP((uint64_t) __bss_end, PAGE_SIZE);

    /* .text: RX (no NX, no W) */
    if (t1 > t0)
    {
        map_range_4k(pml4,
                     t0,
                     virt_to_phys_kernel(t0, kphys_base, kvirt_base),
                     t1 - t0,
                     PTE_P);
    }

    /* .rodata: R, NX */
    if (r1 > r0)
    {
        map_range_4k(pml4,
                     r0,
                     virt_to_phys_kernel(r0, kphys_base, kvirt_base),
                     r1 - r0,
                     PTE_P | PTE_NX);
    }

    /* .data + .bss: RW, NX */
    if (d1 > d0)
    {
        map_range_4k(pml4,
                     d0,
                     virt_to_phys_kernel(d0, kphys_base, kvirt_base),
                     d1 - d0,
                     PTE_P | PTE_W | PTE_NX);
    }
}

/* HHDM: map only useful ranges (skip giant RESERVED/PCIE MMIO holes),
   and prefer 2MiB pages for efficiency. */
static void map_hhdm(struct page_table *pml4)
{
    if (!g_hhdm)
        return;

    if (!memmap_req.response || memmap_req.response->entry_count == 0)
        panic("paging_bootstrap: memmap missing");

    for (size_t i = 0; i < memmap_req.response->entry_count; i++)
    {
        struct limine_memmap_entry *e = memmap_req.response->entries[i];

        bool include = false;
        switch (e->type)
        {
            case LIMINE_MEMMAP_USABLE:
            case LIMINE_MEMMAP_BOOTLOADER_RECLAIMABLE:
            case LIMINE_MEMMAP_ACPI_RECLAIMABLE:
            case LIMINE_MEMMAP_ACPI_NVS:
            case LIMINE_MEMMAP_KERNEL_AND_MODULES:
            case LIMINE_MEMMAP_FRAMEBUFFER:
                include = true;
                break;
            default:
                include = false;
                break;
        }
        if (!include)
            continue;

        uint64_t base = ALIGN_DOWN(e->base, PAGE_SIZE);
        uint64_t end  = ALIGN_UP(e->base + e->length, PAGE_SIZE);
        if (end <= base)
            continue;

        uint64_t len = end - base;
        uint64_t va  = g_hhdm + base;

        map_range_huge_first(pml4, va, base, len, /*base_flags=*/0);
    }
}

/* Public: build and install new PML4 */
void paging_bootstrap(void)
{
    printf("%s\n", "Checking for HHDM... ");
    /* HHDM offset from Limine (must be nonzero for phys->virt table zeroing) */
    if (hhdm_request.response)
        g_hhdm = hhdm_request.response->offset;
    else
        g_hhdm = 0;

    if (g_hhdm == 0)
        panic("paging_bootstrap: HHDM missing; add fallback direct-map if "
              "needed");

    /* Allocate PML4 and build essential mappings */
    g_pml4_virt = alloc_table(&g_pml4_phys);

    printf("%s\n", "Building Minimal Mapping... ");
    map_identity_minimal(g_pml4_virt);
    printf("%s\n", "Mapping Kernel Higher Half... ");
    map_kernel_higher_half(g_pml4_virt);
    printf("%s\n", "Mapping HHDM... ");
    map_hhdm(g_pml4_virt);

    /* Install CR3 */
    printf("%s\n", "Installing CR3 Data... ");
    printf("%s", "INFO: CR3 Value: ");
    printf("0x%llx\n", g_pml4_phys);
    writeCR3(g_pml4_phys);
}
