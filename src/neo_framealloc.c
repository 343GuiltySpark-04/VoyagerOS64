/**
 * VoyagerOS64 - Bitmap frame allocator (Limine, x86_64)
 * Clean rewrite: relocation-safe, guarded, spinlocked.
 */
#include "include/paging/neo_framealloc.h" // must typedef paddr_t = uint64_t
#include "include/limine.h"
#include "include/lock.h" // SPINLOCK_INIT, spinlock_acquire/release
#include "include/panic.h"
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

/* Limine requests are defined ONCE in a separate TU (e.g.,
   boot/limine_requests.c). Here we only extern them. */
extern volatile struct limine_memmap_request         memmap_req;
extern volatile struct limine_kernel_address_request Kaddress_req;

extern volatile struct limine_hhdm_request hhdm_request;

extern volatile struct limine_module_request module_request;

/* Early allocator API (already in your tree) */
extern void    early_init(void);
extern paddr_t early_alloc_page(void);
extern void    early_reserved_region(paddr_t *base, size_t *pages);

#define PAGE_SIZE 4096ull
#define ALIGN_UP(x, a) (((x) + ((a) -1)) & ~((a) -1))
#define ALIGN_DOWN(x, a) ((x) & ~((a) -1))

/* Kernel VMA bounds from linker script (virtual addresses) */
extern char __kernel_start[], __kernel_end[];

/* Panic (your existing banner/trace) */
extern void panic(const char *fmt, ...);

/* --- tiny utils --- */
static inline void memzero(void *p, size_t n)
{
    volatile uint8_t *d = (volatile uint8_t *) p;
    for (size_t i = 0; i < n; i++)
        d[i] = 0;
}
static inline void memset8(void *p, uint8_t v, size_t n)
{
    volatile uint8_t *d = (volatile uint8_t *) p;
    for (size_t i = 0; i < n; i++)
        d[i] = v;
}

/* --- globals --- */
static uint8_t   *g_alloc_bm    = 0; /* 1 = allocated/reserved, 0 = free */
static uint8_t   *g_usable_bm   = 0; /* 1 = USABLE by memmap, 0 = not usable */
static size_t     g_frame_count = 0;
static uint64_t   g_max_phys    = 0;
static size_t     g_free_count  = 0;
static size_t     g_hint        = 0;
static spinlock_t g_memlock     = SPINLOCK_INIT;

/* --- bit ops --- */
static inline void bm_set(uint8_t *bm, size_t i)
{
    bm[i >> 3] |= (uint8_t) (1u << (i & 7));
}
static inline void bm_clear(uint8_t *bm, size_t i)
{
    bm[i >> 3] &= (uint8_t) ~(1u << (i & 7));
}
static inline bool bm_test(const uint8_t *bm, size_t i)
{
    return (bm[i >> 3] >> (i & 7)) & 1u;
}

/* --- helpers --- */
static inline size_t frame_index_of(paddr_t p)
{
    return (size_t) (p / PAGE_SIZE);
}
static inline paddr_t frame_addr_of(size_t i)
{
    return (paddr_t) (i * PAGE_SIZE);
}

static void compute_max_phys_and_count(void)
{
    if (!memmap_req.response || memmap_req.response->entry_count == 0)
        panic("frame_init: Limine memmap missing");
    g_max_phys = 0;
    for (size_t i = 0; i < memmap_req.response->entry_count; i++)
    {
        const struct limine_memmap_entry *e   = memmap_req.response->entries[i];
        uint64_t                          end = e->base + e->length;
        if (end > g_max_phys)
            g_max_phys = end;
    }
    g_frame_count = (size_t) (ALIGN_UP(g_max_phys, PAGE_SIZE) / PAGE_SIZE);
    if (g_frame_count == 0)
        panic("frame_init: zero frames");
}

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
            bm_set(g_usable_bm, i);
        if (bm_test(g_alloc_bm, i))
        {
            bm_clear(g_alloc_bm, i);
            g_free_count++;
        }
    }
}

/* --- public init --- */
void frame_init(void)
{
    /* 1) derive topology */
    compute_max_phys_and_count();

    /* 2) allocate bitmaps from early allocator (separate blocks) */
    if (!hhdm_request.response)
        panic("frame_init: HHDM request missing");
    uint64_t hhdm = hhdm_request.response->offset;

    size_t bm_bytes = (g_frame_count + 7) / 8; /* exact bytes needed */
    size_t bm_pages = (size_t) ALIGN_UP(bm_bytes, PAGE_SIZE) / PAGE_SIZE;

    /* alloc_bm */
    paddr_t a_phys = early_alloc_page();
    for (size_t p = 1; p < bm_pages; p++)
        early_alloc_page();
    g_alloc_bm = (uint8_t *) (a_phys + hhdm);

    /* usable_bm */
    paddr_t b_phys = early_alloc_page();
    for (size_t p = 1; p < bm_pages; p++)
        early_alloc_page();
    g_usable_bm = (uint8_t *) (b_phys + hhdm);

    /* 3) init bitmaps: default all reserved, then free-up USABLE ranges */
    memset8(g_alloc_bm, 0xFF, bm_bytes);
    memzero(g_usable_bm, bm_bytes);
    g_free_count = 0;

    for (size_t i = 0; i < memmap_req.response->entry_count; i++)
    {
        const struct limine_memmap_entry *e = memmap_req.response->entries[i];
        if (e->type == LIMINE_MEMMAP_USABLE)
            mark_range_usable_and_free(e->base, e->base + e->length);
    }

    /* 4) re-reserve kernel (relocation-safe using Limine bases + VMA symbols)
     */
    if (!Kaddress_req.response)
        panic("frame_init: kernel address response missing");
    uint64_t kphys_base = Kaddress_req.response->physical_base;
    uint64_t kvirt_base = Kaddress_req.response->virtual_base;

    uint64_t kphys_begin =
        kphys_base + ((uint64_t) __kernel_start - kvirt_base);
    uint64_t kphys_end = kphys_base + ((uint64_t) __kernel_end - kvirt_base);
    mark_range_allocated((paddr_t) kphys_begin, (paddr_t) kphys_end);

    /* 5) reserve modules (physical addresses provided by Limine) */
    if (module_request.response)
    {
        for (size_t i = 0; i < module_request.response->module_count; i++)
        {
            struct limine_file *m = module_request.response->modules[i];
            mark_range_allocated((paddr_t) m->address,
                                 (paddr_t) (m->address + m->size));
        }
    }

    /* 6) reserve early allocator consumption */
    paddr_t early_base  = 0;
    size_t  early_pages = 0;
    early_reserved_region(&early_base, &early_pages);
    if (early_pages)
        mark_range_allocated(early_base, early_base + early_pages * PAGE_SIZE);

    /* ready */
    g_hint = 0;
}

/* --- public API --- */
paddr_t frame_alloc(void)
{
    spinlock_acquire(&g_memlock);
    if (g_free_count == 0)
    {
        spinlock_release(&g_memlock);
        panic("frame_alloc: OOM");
    }
    size_t start = g_hint;
    for (size_t pass = 0; pass < 2; pass++)
    {
        size_t i   = (pass == 0) ? start : 0;
        size_t end = (pass == 0) ? g_frame_count : start;
        for (; i < end; i++)
        {
            if (bm_test(g_usable_bm, i) && !bm_test(g_alloc_bm, i))
            {
                bm_set(g_alloc_bm, i);
                g_free_count--;
                g_hint    = i + 1;
                paddr_t p = frame_addr_of(i);
                spinlock_release(&g_memlock);
                return p;
            }
        }
    }
    spinlock_release(&g_memlock);
    panic("frame_alloc: free count nonzero but no free frame found (bitmap "
          "corruption?)");
}

void n_frame_free(paddr_t paddr)
{
    if (paddr & (PAGE_SIZE - 1))
        panic("frame_free: unaligned paddr=0x%llx", (unsigned long long) paddr);
    if (paddr >= g_max_phys)
        panic("frame_free: out-of-bounds paddr=0x%llx",
              (unsigned long long) paddr);

    size_t i = frame_index_of(paddr);
    spinlock_acquire(&g_memlock);

    if (!bm_test(g_usable_bm, i))
    {
        spinlock_release(&g_memlock);
        panic("frame_free: non-usable paddr=0x%llx",
              (unsigned long long) paddr);
    }
    if (!bm_test(g_alloc_bm, i))
    {
        spinlock_release(&g_memlock);
        panic("frame_free: double-free paddr=0x%llx",
              (unsigned long long) paddr);
    }

    bm_clear(g_alloc_bm, i);
    g_free_count++;
    if (i < g_hint)
        g_hint = i;
    spinlock_release(&g_memlock);
}

bool frame_is_allocated(paddr_t paddr)
{
    if ((paddr & (PAGE_SIZE - 1)) || paddr >= g_max_phys)
        return true;
    return bm_test(g_alloc_bm, frame_index_of(paddr));
}

size_t frame_total_count(void)
{
    return g_frame_count;
}
size_t frame_free_count(void)
{
    return g_free_count;
}
