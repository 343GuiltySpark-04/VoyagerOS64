/**
 * Copyright (c) 2025 Tristan Adams
 * Released under MIT License.
 */

#include "../include/paging/paging_bootstrap.h"
#include "../include/KernelUtils.h"
#include "../include/early_alloc.h"
#include "../include/limine.h"
#include "../include/printf.h"
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

/* ------------------------------------------------------------------ */
/* Limine requests (DEFINED ONCE in limine_requests.c)                */
/* ------------------------------------------------------------------ */
extern volatile struct limine_memmap_request         memmap_req;
extern volatile struct limine_hhdm_request           hhdm_request;
extern volatile struct limine_kernel_address_request Kaddress_req;

/* Linker symbols: VIRTUAL kernel bounds */
extern char __kernel_start[], __kernel_end[];
extern char __text_start[], __text_end[];
extern char __rodata_start[], __rodata_end[];
extern char __data_start[], __bss_end[];

/* Panic + asm stubs */
extern void     panic(const char *fmt, ...);
extern uint64_t readCR3(void);
extern void     writeCR3(uint64_t arg);
extern uint64_t readRSP(void);

/* Retained for existing debug/PMM plumbing. */
uint64_t g_alloc_bm_phys_base  = 0;
uint64_t g_alloc_bm_bytes      = 0;
uint64_t g_usable_bm_phys_base = 0;
uint64_t g_usable_bm_bytes     = 0;
uint64_t framebuffer_phys      = 0;
uint64_t framebuffer_bytes     = 0;

/* ------------------------------------------------------------------ */
/* x86_64 paging bits / helpers                                       */
/* ------------------------------------------------------------------ */
#define PTE_P (1ull << 0)
#define PTE_W (1ull << 1)
#define PTE_U (1ull << 2)
#define PTE_PS (1ull << 7)
#define PTE_NX (1ull << 63)

#define PAGE_SIZE 4096ull
#define PAGE_2M_SIZE (2ull * 1024 * 1024)
#define PAGE_1G_SIZE (1ull << 30)

#define ADDR_MASK 0x000FFFFFFFFFF000ull
#define ADDR_MASK_2M 0x000FFFFFFFFFE000ull
#define ADDR_MASK_1G 0x000FFFFFC0000000ull

#define ALIGN_UP(x, a) (((x) + ((a) -1)) & ~((a) -1))
#define ALIGN_DOWN(x, a) ((x) & ~((a) -1))

struct __attribute__((aligned(0x1000))) page_table
{
    uint64_t e[512];
};

static uint64_t           g_pml4_phys = 0;
static struct page_table *g_pml4_virt = 0;
static uint64_t           g_hhdm      = 0;

uint64_t paging_pml4_phys(void)
{
    return g_pml4_phys;
}

void *paging_pml4_virt(void)
{
    return (void *) g_pml4_virt;
}

uint64_t get_total_usable_mem_bytes(void)
{
    if (!memmap_req.response || memmap_req.response->entry_count == 0)
        return 0;

    uint64_t total = 0;
    for (size_t i = 0; i < memmap_req.response->entry_count; i++)
    {
        struct limine_memmap_entry *e = memmap_req.response->entries[i];
        if (e->type == LIMINE_MEMMAP_USABLE)
            total += e->length;
    }
    return total;
}

static uint64_t get_max_phys(void)
{
    uint64_t max_phys = 0;

    if (!memmap_req.response)
        return 0;

    for (size_t i = 0; i < memmap_req.response->entry_count; i++)
    {
        struct limine_memmap_entry *e   = memmap_req.response->entries[i];
        uint64_t                    end = e->base + e->length;
        if (end > max_phys)
            max_phys = end;
    }

    return max_phys;
}

static inline void memzero(void *p, size_t n)
{
    volatile uint8_t *d = (volatile uint8_t *) p;
    for (size_t i = 0; i < n; i++)
        d[i] = 0;
}

static inline void *phys_to_virt(uint64_t phys)
{
    return (void *) (uintptr_t) (phys + g_hhdm);
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

/* ------------------------------------------------------------------ */
/* Table allocation                                                   */
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

/* This helper is ONLY valid for non-leaf entries. A present PS entry is a
 * huge-page leaf, not another page-table pointer. Treating it as a child
 * table is memory corruption, so fail loudly if a caller ever tries. */
static struct page_table *
get_or_make_next(struct page_table *cur, uint64_t idx, uint64_t *phys_out)
{
    uint64_t e = cur->e[idx];

    if (e & PTE_P)
    {
        if (e & PTE_PS)
            panic("paging_bootstrap: attempted descent through huge-page leaf");

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
    return get_or_make_next(pdpt, pdpti, pd_phys_out);
}

/* ------------------------------------------------------------------ */
/* Mapping helpers                                                    */
/* ------------------------------------------------------------------ */
static void
map_4k(struct page_table *pml4, uint64_t va, uint64_t pa, uint64_t flags)
{
    uint64_t idx[4];
    uint64_t junk;
    vaddr_indices(va, idx);

    struct page_table *pdpt = get_or_make_next(pml4, idx[0], &junk);
    struct page_table *pd   = get_or_make_next(pdpt, idx[1], &junk);
    struct page_table *pt   = get_or_make_next(pd, idx[2], &junk);

    pt->e[idx[3]] = (pa & ADDR_MASK) | (flags & ~ADDR_MASK) | PTE_P;
}

static void
map_2m(struct page_table *pml4, uint64_t va, uint64_t pa, uint64_t flags)
{
    uint64_t idx[4];
    uint64_t pd_phys;
    vaddr_indices(va, idx);

    struct page_table *pd = get_or_make_pd(pml4, idx[0], idx[1], &pd_phys);
    pd->e[idx[2]] =
        (pa & ADDR_MASK_2M) | (flags & ~ADDR_MASK) | PTE_P | PTE_W | PTE_PS;
}

static void
map_1g(struct page_table *pml4, uint64_t va, uint64_t pa, uint64_t flags)
{
    uint64_t idx[4];
    uint64_t junk;
    vaddr_indices(va, idx);

    struct page_table *pdpt = get_or_make_next(pml4, idx[0], &junk);
    pdpt->e[idx[1]] =
        (pa & ADDR_MASK_1G) | (flags & ~ADDR_MASK) | PTE_P | PTE_W | PTE_PS;
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
        map_4k(pml4, va, pa, base_flags | PTE_W | PTE_NX);
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
        map_4k(pml4, va, pa, base_flags | PTE_W | PTE_NX);
        pa += PAGE_SIZE;
        va += PAGE_SIZE;
    }
}

/* ------------------------------------------------------------------ */
/* Kernel + HHDM builders                                             */
/* ------------------------------------------------------------------ */
static void map_identity_minimal(struct page_table *pml4)
{
    for (uint64_t p = 0; p < PAGE_2M_SIZE; p += PAGE_SIZE)
        map_4k(pml4, p, p, PTE_W | PTE_NX);
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
                     0); /* RX */

    if (r1 > r0)
        map_range_4k(pml4,
                     r0,
                     virt_to_phys_kernel(r0, kphys_base, kvirt_base),
                     r1 - r0,
                     PTE_NX); /* R */

    if (d1 > d0)
        map_range_4k(pml4,
                     d0,
                     virt_to_phys_kernel(d0, kphys_base, kvirt_base),
                     d1 - d0,
                     PTE_W | PTE_NX); /* RW */
}

static void map_hhdm(struct page_table *pml4)
{
    if (!g_hhdm)
        panic("paging_bootstrap: HHDM offset missing");

    if (!memmap_req.response || memmap_req.response->entry_count == 0)
        panic("paging_bootstrap: memmap missing");

    /* Build Voyager-owned HHDM mappings. Do not shallow-clone Limine's
     * upper-half tables: sharing lower-level paging structures means any
     * retrofit edit can mutate the currently active Limine address space. */
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

        if (!include)
            continue;

        uint64_t base = ALIGN_DOWN(e->base, PAGE_SIZE);
        uint64_t end  = ALIGN_UP(e->base + e->length, PAGE_SIZE);

        if (end <= base)
            continue;

        map_range_huge_first(pml4, g_hhdm + base, base, end - base, 0);
    }

    /* Always keep the first 2 MiB addressable through HHDM. This is written
     * last deliberately: it may replace fragmented low-page mappings with
     * one equivalent 2 MiB leaf, but nothing later is allowed to descend
     * through that leaf. */
    map_2m(pml4, g_hhdm, 0, PTE_NX);
}

/* ------------------------------------------------------------------ */
/* VA walker + diagnostics                                            */
/* ------------------------------------------------------------------ */
static bool walk_va_to_pa(uint64_t           va,
                          struct page_table *pml4_root,
                          uint64_t          *pa_out,
                          int               *level_out)
{
    uint64_t idx[4];
    vaddr_indices(va, idx);

    uint64_t pml4e = pml4_root->e[idx[0]];
    if (!(pml4e & PTE_P))
        return false;

    struct page_table *pdpt =
        (struct page_table *) phys_to_virt(pml4e & ADDR_MASK);
    uint64_t pdpte = pdpt->e[idx[1]];
    if (!(pdpte & PTE_P))
        return false;

    if (pdpte & PTE_PS)
    {
        if (level_out)
            *level_out = 3;
        if (pa_out)
            *pa_out = (pdpte & ADDR_MASK_1G) | (va & (PAGE_1G_SIZE - 1));
        return true;
    }

    struct page_table *pd =
        (struct page_table *) phys_to_virt(pdpte & ADDR_MASK);
    uint64_t pde = pd->e[idx[2]];
    if (!(pde & PTE_P))
        return false;

    if (pde & PTE_PS)
    {
        if (level_out)
            *level_out = 2;
        if (pa_out)
            *pa_out = (pde & ADDR_MASK_2M) | (va & (PAGE_2M_SIZE - 1));
        return true;
    }

    struct page_table *pt = (struct page_table *) phys_to_virt(pde & ADDR_MASK);
    uint64_t           pte = pt->e[idx[3]];
    if (!(pte & PTE_P))
        return false;

    if (level_out)
        *level_out = 1;
    if (pa_out)
        *pa_out = (pte & ADDR_MASK) | (va & (PAGE_SIZE - 1));

    return true;
}

static void probe_va(const char *label, uint64_t va, struct page_table *tbl)
{
    int      lvl = 0;
    uint64_t pa  = 0;

    if (!walk_va_to_pa(va, tbl, &pa, &lvl))
    {
        printf("  probe %-12s: VA=0x%llx MISSING\n",
               label,
               (unsigned long long) va);
        return;
    }

    printf("  probe %-12s: VA=0x%llx -> PA=0x%llx (lvl %d)\n",
           label,
           (unsigned long long) va,
           (unsigned long long) pa,
           lvl);
}

static void
probe_hhdm_phys(const char *label, uint64_t phys, struct page_table *pml4)
{
    uint64_t va  = g_hhdm + phys;
    uint64_t pa  = 0;
    int      lvl = 0;

    if (!walk_va_to_pa(va, pml4, &pa, &lvl))
    {
        printf("HHDM %s: phys=0x%llx VA=0x%llx MISSING\n",
               label,
               (unsigned long long) phys,
               (unsigned long long) va);
        return;
    }

    printf("HHDM %s: phys=0x%llx VA=0x%llx -> PA=0x%llx (lvl %d)\n",
           label,
           (unsigned long long) phys,
           (unsigned long long) va,
           (unsigned long long) pa,
           lvl);
}

static void scan_hhdm_range(uint64_t           phys_start,
                            uint64_t           length,
                            uint64_t           stride,
                            struct page_table *pml4)
{
    if (stride == 0)
        stride = PAGE_2M_SIZE;

    uint64_t end        = phys_start + length;
    int      prev_state = -1;
    uint64_t run_start  = phys_start;

    for (uint64_t phys = phys_start; phys < end; phys += stride)
    {
        bool present = walk_va_to_pa(g_hhdm + phys, pml4, NULL, NULL);
        int  state   = present ? 1 : 0;

        if (prev_state == -1)
        {
            prev_state = state;
            run_start  = phys;
        }
        else if (state != prev_state)
        {
            printf("HHDM scan: [%#llx .. %#llx) %s\n",
                   (unsigned long long) run_start,
                   (unsigned long long) phys,
                   prev_state ? "PRESENT" : "MISSING");
            prev_state = state;
            run_start  = phys;
        }
    }

    printf("HHDM scan: [%#llx .. %#llx) %s\n",
           (unsigned long long) run_start,
           (unsigned long long) end,
           prev_state > 0 ? "PRESENT" : "MISSING");
}

/* ------------------------------------------------------------------ */
/* Safe coverage enforcement                                          */
/* ------------------------------------------------------------------ */
static void ensure_hhdm_cover_phys_page(struct page_table *pml4,
                                        uint64_t           phys_page)
{
    uint64_t page = ALIGN_DOWN(phys_page, PAGE_SIZE);
    uint64_t va   = g_hhdm + page;

    /* A huge-page mapping is already sufficient coverage. Never replace or
     * descend through it just to obtain a 4 KiB mapping. */
    if (walk_va_to_pa(va, pml4, NULL, NULL))
        return;

    map_4k(pml4, va, page, PTE_W | PTE_NX);
}

static void ensure_hhdm_cover_phys_range(struct page_table *pml4,
                                         uint64_t           phys,
                                         uint64_t           len)
{
    if (len == 0)
        return;

    uint64_t page = ALIGN_DOWN(phys, PAGE_SIZE);
    uint64_t end  = ALIGN_UP(phys + len, PAGE_SIZE);

    for (; page < end; page += PAGE_SIZE)
        ensure_hhdm_cover_phys_page(pml4, page);
}

/* ------------------------------------------------------------------ */
/* Pre-switch validation + install                                    */
/* ------------------------------------------------------------------ */
static void pre_switch_audit_and_patch(struct page_table *new_pml4,
                                       uint64_t           new_pml4_phys)
{
    uint64_t old_cr3  = readCR3() & ADDR_MASK;
    uint64_t rsp_va   = readRSP();
    uint64_t rsp_phys = 0;
    uint64_t max_phys = get_max_phys();

    if (rsp_va >= g_hhdm && rsp_va < g_hhdm + max_phys)
    {
        rsp_phys = rsp_va - g_hhdm;
    }
    else if (!walk_va_to_pa(rsp_va, new_pml4, &rsp_phys, NULL))
    {
        panic("paging_bootstrap: stack VA missing before CR3 switch");
    }

    /* These calls are deliberately idempotent. Existing 2 MiB/1 GiB HHDM
     * leaves count as valid coverage and are left untouched. */
    ensure_hhdm_cover_phys_range(new_pml4, 0, PAGE_2M_SIZE);
    ensure_hhdm_cover_phys_page(new_pml4, rsp_phys);
    ensure_hhdm_cover_phys_page(new_pml4, old_cr3);
    ensure_hhdm_cover_phys_page(new_pml4, new_pml4_phys);

    printf("Pre-switch validation:\n");
    probe_va("stack", rsp_va, new_pml4);
    probe_va(
        "rip", (uint64_t) (uintptr_t) &pre_switch_audit_and_patch, new_pml4);
    probe_va("HHDM+0", g_hhdm, new_pml4);
    probe_va("HHDM+oldCR3", g_hhdm + old_cr3, new_pml4);
    probe_va("HHDM+newCR3", g_hhdm + new_pml4_phys, new_pml4);

    if (!walk_va_to_pa(rsp_va, new_pml4, NULL, NULL))
        panic("paging_bootstrap: stack missing after patching");

    if (!walk_va_to_pa((uint64_t) (uintptr_t) &pre_switch_audit_and_patch,
                       new_pml4,
                       NULL,
                       NULL))
        panic("paging_bootstrap: kernel RIP missing after patching");

    if (!walk_va_to_pa(g_hhdm, new_pml4, NULL, NULL))
        panic("paging_bootstrap: HHDM+0 missing after patching");

    if (!walk_va_to_pa(g_hhdm + old_cr3, new_pml4, NULL, NULL))
        panic("paging_bootstrap: old CR3 page missing from HHDM");

    if (!walk_va_to_pa(g_hhdm + new_pml4_phys, new_pml4, NULL, NULL))
        panic("paging_bootstrap: new CR3 page missing from HHDM");
}

static void install_new_cr3(uint64_t new_pml4_phys, struct page_table *new_pml4)
{
    pre_switch_audit_and_patch(new_pml4, new_pml4_phys);

    uint64_t old_cr3 = readCR3();
    printf("Installing CR3... old=0x%llx new=0x%llx\n",
           (unsigned long long) old_cr3,
           (unsigned long long) new_pml4_phys);

    writeCR3(new_pml4_phys);

    uint64_t cr3_after = readCR3();
    printf("CR3 after install: 0x%llx\n", (unsigned long long) cr3_after);

    asm volatile("push %%rax; pop %%rax" ::: "rax", "memory");
    volatile uint8_t *text_probe =
        (volatile uint8_t *) (uintptr_t) &install_new_cr3;
    (void) *text_probe;

    printf("CR3 install validation complete.\n");
}

/* ------------------------------------------------------------------ */
/* Public bootstrap                                                   */
/* ------------------------------------------------------------------ */
void paging_bootstrap(void)
{
    if (!hhdm_request.response)
        panic("paging_bootstrap: HHDM response missing");

    g_hhdm = hhdm_request.response->offset;
    if (g_hhdm == 0)
        panic("paging_bootstrap: HHDM offset is zero");

    /* New root and every lower-level table are Voyager-owned. We use the
     * currently active Limine HHDM only to access/zero these physical pages
     * while constructing the replacement address space. */
    g_pml4_virt = alloc_table(&g_pml4_phys);

    map_identity_minimal(g_pml4_virt);
    map_kernel_higher_half(g_pml4_virt);
    map_hhdm(g_pml4_virt);

    printf("INFO: CR3 to install: 0x%llx\n", (unsigned long long) g_pml4_phys);
    printf("INFO: RSP: 0x%llx\n", (unsigned long long) readRSP());

    printf("Memory Total: %iMiB\n", bytes_to_mib(get_total_usable_mem_bytes()));

    printf(">>> Probing HHDM <<<\n");
    printf("--------------------------------\n");

    probe_hhdm_phys("low+0", 0, g_pml4_virt);
    probe_hhdm_phys("old CR3 page", readCR3() & ADDR_MASK, g_pml4_virt);
    probe_hhdm_phys("new CR3 page", g_pml4_phys, g_pml4_virt);

    scan_hhdm_range(0, 16ull * 1024 * 1024, PAGE_2M_SIZE, g_pml4_virt);

    printf("--------------------------------\n");
    printf(">>> HHDM Probe Done <<<\n");

    install_new_cr3(g_pml4_phys, g_pml4_virt);
}
