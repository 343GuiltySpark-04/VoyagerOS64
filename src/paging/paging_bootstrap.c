/**
 * Copyright (c) 2025 Tristan Adams
 * Released under MIT License.
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

/* asm stubs */
extern uint64_t readCR3();
extern void     writeCR3(uint64_t arg);
extern uint64_t readRSP();
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

/* Globals */
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

/* Tiny memset */
static inline void memzero(void *p, size_t n)
{
    volatile uint8_t *d = (volatile uint8_t *) p;
    for (size_t i = 0; i < n; i++)
        d[i] = 0;
}

/* phys <-> virt */
static inline void *phys_to_virt(uint64_t phys)
{
    return (void *) (phys + g_hhdm);
}

/* ------------------------------------------------------------------ */
/* Table alloc + walker helpers                                       */
/* ------------------------------------------------------------------ */
static struct page_table *alloc_table(uint64_t *out_phys)
{
    uint64_t phys = early_alloc_page();
    if (g_hhdm == 0)
        panic("paging_bootstrap: HHDM required for table zeroing");
    struct page_table *pt = (struct page_table *) phys_to_virt(phys);
    memzero(pt, sizeof(*pt));
    if (out_phys)
        *out_phys = phys;
    return pt;
}

static inline void vaddr_indices(uint64_t v, uint64_t idx[4])
{
    idx[0] = (v >> 39) & 0x1FF;
    idx[1] = (v >> 30) & 0x1FF;
    idx[2] = (v >> 21) & 0x1FF;
    idx[3] = (v >> 12) & 0x1FF;
}

static inline uint64_t
virt_to_phys_kernel(uint64_t vaddr, uint64_t kphys_base, uint64_t kvirt_base)
{
    return kphys_base + (vaddr - kvirt_base);
}

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

/* ------------------------------------------------------------------ */
/* Mapping helpers                                                    */
/* ------------------------------------------------------------------ */
static void
map_4k(struct page_table *pml4, uint64_t va, uint64_t pa, uint64_t flags)
{
    uint64_t idx[4];
    vaddr_indices(va, idx);
    uint64_t           junk;
    struct page_table *pdpt = get_or_make_next(pml4, idx[0], &junk);
    struct page_table *pd   = get_or_make_next(pdpt, idx[1], &junk);
    struct page_table *pt   = get_or_make_next(pd, idx[2], &junk);
    pt->e[idx[3]]           = (pa & ADDR_MASK) | (flags & ~ADDR_MASK);
}

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

static void map_range_4k(struct page_table *pml4,
                         uint64_t           va,
                         uint64_t           pa,
                         uint64_t           len,
                         uint64_t           flags)
{
    uint64_t end = va + len;
    for (; va < end; va += PAGE_SIZE, pa += PAGE_SIZE)
        map_4k(pml4, va, pa, flags);
}

static void map_range_huge_first(struct page_table *pml4,
                                 uint64_t           va,
                                 uint64_t           pa,
                                 uint64_t           len,
                                 uint64_t           base_flags)
{
    uint64_t end = pa + len;
    while (pa < end && (((pa | va) & (PAGE_2M_SIZE - 1)) != 0))
    {
        map_4k(pml4, va, pa, base_flags | PTE_P | PTE_W | PTE_NX);
        pa += PAGE_SIZE;
        va += PAGE_SIZE;
    }
    while (pa + PAGE_2M_SIZE <= end)
    {
        map_2m(pml4, va, pa, base_flags | PTE_NX);
        pa += PAGE_2M_SIZE;
        va += PAGE_2M_SIZE;
    }
    while (pa < end)
    {
        map_4k(pml4, va, pa, base_flags | PTE_P | PTE_W | PTE_NX);
        pa += PAGE_SIZE;
        va += PAGE_SIZE;
    }
}

/* ------------------------------------------------------------------ */
/* Kernel + HHDM mappings                                             */
/* ------------------------------------------------------------------ */
static void map_identity_minimal(struct page_table *pml4)
{
    for (uint64_t p = 0; p < 0x200000; p += PAGE_SIZE)
        map_4k(pml4, p, p, PTE_P | PTE_W | PTE_NX);
}

static void map_kernel_higher_half(struct page_table *pml4)
{
    if (!Kaddress_req.response)
        panic("paging_bootstrap: kernel address request missing");

    uint64_t kphys_base = Kaddress_req.response->physical_base;
    uint64_t kvirt_base = Kaddress_req.response->virtual_base;

    uint64_t t0 = ALIGN_DOWN((uint64_t) __text_start, PAGE_SIZE);
    uint64_t t1 = ALIGN_UP((uint64_t) __text_end, PAGE_SIZE);
    uint64_t r0 = ALIGN_DOWN((uint64_t) __rodata_start, PAGE_SIZE);
    uint64_t r1 = ALIGN_UP((uint64_t) __rodata_end, PAGE_SIZE);
    uint64_t d0 = ALIGN_DOWN((uint64_t) __data_start, PAGE_SIZE);
    uint64_t d1 = ALIGN_UP((uint64_t) __bss_end, PAGE_SIZE);

    if (t1 > t0)
        map_range_4k(pml4,
                     t0,
                     virt_to_phys_kernel(t0, kphys_base, kvirt_base),
                     t1 - t0,
                     PTE_P);

    if (r1 > r0)
        map_range_4k(pml4,
                     r0,
                     virt_to_phys_kernel(r0, kphys_base, kvirt_base),
                     r1 - r0,
                     PTE_P | PTE_NX);

    if (d1 > d0)
        map_range_4k(pml4,
                     d0,
                     virt_to_phys_kernel(d0, kphys_base, kvirt_base),
                     d1 - d0,
                     PTE_P | PTE_W | PTE_NX);
}

static void map_hhdm(struct page_table *pml4)
{
    if (!g_hhdm)
        return;
    if (!memmap_req.response || memmap_req.response->entry_count == 0)
        panic("paging_bootstrap: memmap missing");

    for (size_t i = 0; i < memmap_req.response->entry_count; i++)
    {
        struct limine_memmap_entry *e       = memmap_req.response->entries[i];
        bool                        include = false;
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
                break;
        }
        map_range_huge_first(pml4, g_hhdm, 0, 0x200000, 0);
        if (!include)
            continue;
        uint64_t base = ALIGN_DOWN(e->base, PAGE_SIZE);
        uint64_t end  = ALIGN_UP(e->base + e->length, PAGE_SIZE);
        if (end <= base)
            continue;
        uint64_t len = end - base;
        uint64_t va  = g_hhdm + base;
        map_range_huge_first(pml4, va, base, len, 0);
    }
}

/* ------------------------------------------------------------------ */
/* VA walker + probes                                                 */
/* ------------------------------------------------------------------ */
static uint64_t
walk_va_to_pa(uint64_t va, struct page_table *pml4_root, int *level_out)
{
    uint64_t idx[4];
    vaddr_indices(va, idx);
    uint64_t pml4e = pml4_root->e[idx[0]];
    if (!(pml4e & PTE_P))
        return 0;
    struct page_table *pdpt =
        (struct page_table *) phys_to_virt(pml4e & ADDR_MASK);
    uint64_t pdpte = pdpt->e[idx[1]];
    if (!(pdpte & PTE_P))
        return 0;
    if (pdpte & PTE_PS)
    {
        if (level_out)
            *level_out = 3;
        uint64_t base = pdpte & 0x000FFFFFC0000000ull;
        return base | (va & ((1ull << 30) - 1));
    }
    struct page_table *pd =
        (struct page_table *) phys_to_virt(pdpte & ADDR_MASK);
    uint64_t pde = pd->e[idx[2]];
    if (!(pde & PTE_P))
        return 0;
    if (pde & PTE_PS)
    {
        if (level_out)
            *level_out = 2;
        uint64_t base = pde & 0x000FFFFFFFE00000ull;
        return base | (va & ((1ull << 21) - 1));
    }
    struct page_table *pt = (struct page_table *) phys_to_virt(pde & ADDR_MASK);
    uint64_t           pte = pt->e[idx[3]];
    if (!(pte & PTE_P))
        return 0;
    if (level_out)
        *level_out = 1;
    uint64_t base = pte & ADDR_MASK;
    return base | (va & (PAGE_SIZE - 1));
}

static void probe_va(const char *label, uint64_t va, struct page_table *tbl)
{
    int      lvl;
    uint64_t pa = walk_va_to_pa(va, tbl, &lvl);
    if (pa == 0)
    {
        printf("  probe %-12s: VA=0x%llx MISSING\n",
               label,
               (unsigned long long) va);
    }
    else
    {
        printf("  probe %-12s: VA=0x%llx -> PA=0x%llx (lvl %d)\n",
               label,
               (unsigned long long) va,
               (unsigned long long) pa,
               lvl);
    }
}

/* ------------------------------------------------------------------ */
/* Upper-half cloning + CR3 switch                                    */
/* ------------------------------------------------------------------ */
static void clone_upper_half_from_current(struct page_table *new_pml4)
{
    uint64_t           old_cr3_phys = readCR3();
    struct page_table *old_pml4 =
        (struct page_table *) phys_to_virt(old_cr3_phys);
    for (size_t i = 256; i < 512; i++)
    {
        uint64_t e = old_pml4->e[i];
        if (e & PTE_P)
            new_pml4->e[i] = e;
    }
}

static bool pre_switch_validate(struct page_table *new_pml4,
                                uint64_t           new_pml4_phys)
{
    uint64_t old_cr3_phys = readCR3();
    uint64_t rsp          = readRSP();
    uint64_t rip          = (uint64_t) (uintptr_t) &pre_switch_validate;
    printf("Pre-switch validation:\n");
    probe_va("stack", rsp, new_pml4);
    probe_va("rip", rip, new_pml4);
    probe_va("HHDM+0", g_hhdm + 0, new_pml4);
    probe_va("HHDM+oldCR3", g_hhdm + old_cr3_phys, new_pml4);
    probe_va("HHDM+newCR3", g_hhdm + new_pml4_phys, new_pml4);
    return true; /* we just log; panic if something looks wrong */
}

static void install_new_cr3(uint64_t new_pml4_phys, struct page_table *new_pml4)
{
    pre_switch_validate(new_pml4, new_pml4_phys);
    uint64_t old_cr3 = readCR3();
    printf("Installing CR3... old=0x%llx new=0x%llx\n",
           (unsigned long long) old_cr3,
           (unsigned long long) new_pml4_phys);
    writeCR3(new_pml4_phys);
    uint64_t cr3_after = readCR3();
    printf("CR3 after install: 0x%llx\n", (unsigned long long) cr3_after);
    asm volatile("push %%rax; pop %%rax" ::: "rax", "memory");
    volatile uint8_t *p = (volatile uint8_t *) &install_new_cr3;
    (void) *p;
    printf("CR3 install validation complete.\n");
}

/* ------------------------------------------------------------------ */
/* Public bootstrap                                                   */
/* ------------------------------------------------------------------ */
void paging_bootstrap(void)
{
    if (hhdm_request.response)
        g_hhdm = hhdm_request.response->offset;
    if (g_hhdm == 0)
        panic("paging_bootstrap: HHDM missing");

    g_pml4_virt = alloc_table(&g_pml4_phys);

    map_identity_minimal(g_pml4_virt);
    clone_upper_half_from_current(g_pml4_virt);
    map_kernel_higher_half(g_pml4_virt);
    map_hhdm(g_pml4_virt);

    printf("INFO: CR3 to install: 0x%llx\n", (unsigned long long) g_pml4_phys);
    printf("INFO: RSP: 0x%llx\n", (unsigned long long) readRSP());

    install_new_cr3(g_pml4_phys, g_pml4_virt);
}
