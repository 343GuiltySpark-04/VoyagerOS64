/**
 * Copyright (c) 2026 Tristan Adams
 *
 * VoyagerOS64 Stage-2 memory qualification harness.
 */

#include "../include/mm/memtest.h"
#include "../include/limine.h"
#include "../include/mm/kmalloc.h"
#include "../include/paging/neo_framealloc.h"
#include "../include/panic.h"
#include "../include/printf.h"
#include "../include/sched.h"
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define MEMTEST_SLOT_COUNT 128u
#define MEMTEST_PMM_MAX_FRAMES 256u
#define MEMTEST_PAGE_SIZE 4096u
#define MEMTEST_RANDOM_OPS 10000u
#define MEMTEST_RANDOM_MAX_ALLOC 32768u
#define MEMTEST_GROWTH_SLOTS 64u
#define MEMTEST_GROWTH_SIZE 32768u

typedef struct memtest_slot
{
    uint8_t *ptr;
    size_t   size;
    uint8_t  tag;
} memtest_slot_t;

typedef struct memtest_stats
{
    uint64_t operations;
    uint64_t allocations;
    uint64_t frees;
    uint64_t reallocations;
    uint64_t live_bytes;
    uint64_t peak_live_bytes;
} memtest_stats_t;

static memtest_slot_t g_slots[MEMTEST_SLOT_COUNT];
static paddr_t        g_pmm_frames[MEMTEST_PMM_MAX_FRAMES];

extern volatile struct limine_hhdm_request hhdm_request;

static uint8_t payload_pattern(uint8_t tag, size_t offset)
{
    return (uint8_t) (tag ^ (uint8_t) offset ^ (uint8_t) (offset >> 8) ^
                      (uint8_t) (offset >> 16));
}

static uint8_t page_pattern(size_t page_index, size_t offset)
{
    return (uint8_t) ((uint8_t) page_index ^ (uint8_t) offset ^
                      (uint8_t) (offset >> 8) ^ 0xA5u);
}

static void memtest_fail(const char *phase,
                         size_t      slot,
                         size_t      offset,
                         uint8_t     expected,
                         uint8_t     actual)
{
    printf_("%s\n", "");
    printf_("%s\n", "[MEMTEST] FAILURE");
    printf_("%s%s\n", "Phase: ", phase);
    printf_("%s%llu\n", "Slot: ", (unsigned long long) slot);
    printf_("%s%llu\n", "Offset: ", (unsigned long long) offset);
    printf_("%s%u\n", "Expected: ", (unsigned int) expected);
    printf_("%s%u\n", "Actual: ", (unsigned int) actual);
    panic("memtest: data corruption detected");
}

static void clear_slots(void)
{
    for (size_t i = 0; i < MEMTEST_SLOT_COUNT; i++)
    {
        g_slots[i].ptr  = NULL;
        g_slots[i].size = 0;
        g_slots[i].tag  = 0;
    }
}

static void verify_prefix(const char *phase,
                          size_t      slot_index,
                          const uint8_t *ptr,
                          size_t      size,
                          uint8_t     tag)
{
    for (size_t i = 0; i < size; i++)
    {
        uint8_t expected = payload_pattern(tag, i);
        if (ptr[i] != expected)
            memtest_fail(phase, slot_index, i, expected, ptr[i]);
    }
}

static void fill_slot(size_t slot_index)
{
    memtest_slot_t *slot = &g_slots[slot_index];

    for (size_t i = 0; i < slot->size; i++)
        slot->ptr[i] = payload_pattern(slot->tag, i);
}

static void verify_slot(const char *phase, size_t slot_index)
{
    memtest_slot_t *slot = &g_slots[slot_index];

    if (!slot->ptr)
        return;

    verify_prefix(phase, slot_index, slot->ptr, slot->size, slot->tag);
}

static void verify_all(const char *phase)
{
    for (size_t i = 0; i < MEMTEST_SLOT_COUNT; i++)
        verify_slot(phase, i);
}

static void verify_alignment(const char *phase, size_t slot_index, void *ptr)
{
    if (((uintptr_t) ptr & 0xFu) != 0)
    {
        printf_("%s%s\n", "[MEMTEST] Phase: ", phase);
        printf_("%s%llu\n", "Unaligned slot: ",
                (unsigned long long) slot_index);
        printf_("%s%llx\n", "Pointer: 0x",
                (unsigned long long) (uintptr_t) ptr);
        panic("memtest: kmalloc returned a non-16-byte-aligned pointer");
    }
}

static void verify_no_overlap(const char *phase,
                              size_t      slot_index,
                              uint8_t    *ptr,
                              size_t      size)
{
    uintptr_t start = (uintptr_t) ptr;
    uintptr_t end   = start + size;

    if (end < start)
        panic("memtest: allocation address overflow");

    for (size_t i = 0; i < MEMTEST_SLOT_COUNT; i++)
    {
        if (i == slot_index || g_slots[i].ptr == NULL)
            continue;

        uintptr_t other_start = (uintptr_t) g_slots[i].ptr;
        uintptr_t other_end   = other_start + g_slots[i].size;

        if (start < other_end && other_start < end)
        {
            printf_("%s%s\n", "[MEMTEST] Phase: ", phase);
            printf_("%s%llu%s%llu\n",
                    "Allocation overlap between slots ",
                    (unsigned long long) slot_index,
                    " and ",
                    (unsigned long long) i);
            panic("memtest: allocator returned overlapping payloads");
        }
    }
}

static void set_slot(size_t slot_index, uint8_t *ptr, size_t size, uint8_t tag,
                     const char *phase)
{
    if (!ptr)
    {
        printf_("%s%s\n", "[MEMTEST] Allocation failed in phase: ", phase);
        printf_("%s%llu\n", "Requested bytes: ",
                (unsigned long long) size);
        panic("memtest: unexpected allocation failure");
    }

    verify_alignment(phase, slot_index, ptr);

    size_t usable = kmalloc_usable_size(ptr);
    if (usable < size)
    {
        printf_("%s%s\n", "[MEMTEST] Phase: ", phase);
        printf_("%s%llu%s%llu\n",
                "Allocator reported usable size ",
                (unsigned long long) usable,
                " for request ",
                (unsigned long long) size);
        panic("memtest: kmalloc usable size smaller than requested size");
    }

    verify_no_overlap(phase, slot_index, ptr, size);

    g_slots[slot_index].ptr  = ptr;
    g_slots[slot_index].size = size;
    g_slots[slot_index].tag  = tag;
    fill_slot(slot_index);
}

static void release_slot(const char *phase, size_t slot_index)
{
    if (!g_slots[slot_index].ptr)
        return;

    verify_slot(phase, slot_index);
    kfree(g_slots[slot_index].ptr);
    g_slots[slot_index].ptr  = NULL;
    g_slots[slot_index].size = 0;
    g_slots[slot_index].tag  = 0;
}

static void pmm_roundtrip_test(void)
{
    printf_("%s\n", "[MEMTEST] 1/6 neo-PMM round-trip...");

    if (!hhdm_request.response)
        panic("memtest: HHDM unavailable");

    size_t before = frame_free_count();
    size_t count  = MEMTEST_PMM_MAX_FRAMES;

    if (before <= count + 32u)
        count = before / 4u;
    if (count < 8u)
        panic("memtest: insufficient free frames for PMM qualification");

    uint64_t hhdm = hhdm_request.response->offset;

    for (size_t i = 0; i < count; i++)
    {
        paddr_t p = frame_alloc();

        if ((p & (MEMTEST_PAGE_SIZE - 1u)) != 0)
            panic("memtest: frame_alloc returned an unaligned frame");

        for (size_t j = 0; j < i; j++)
        {
            if (g_pmm_frames[j] == p)
                panic("memtest: frame_alloc returned a duplicate live frame");
        }

        g_pmm_frames[i] = p;
        uint8_t *page = (uint8_t *) (uintptr_t) (p + hhdm);

        for (size_t off = 0; off < MEMTEST_PAGE_SIZE; off++)
            page[off] = page_pattern(i, off);

        if ((i & 31u) == 31u)
            schedule();
    }

    size_t after_alloc = frame_free_count();
    if (after_alloc + count != before)
    {
        printf_("%s%llu%s%llu%s%llu\n",
                "[MEMTEST] PMM count mismatch: before=",
                (unsigned long long) before,
                " after_alloc=",
                (unsigned long long) after_alloc,
                " frames=",
                (unsigned long long) count);
        panic("memtest: PMM accounting mismatch after allocation");
    }

    for (size_t i = 0; i < count; i++)
    {
        uint8_t *page =
            (uint8_t *) (uintptr_t) (g_pmm_frames[i] + hhdm);

        for (size_t off = 0; off < MEMTEST_PAGE_SIZE; off++)
        {
            uint8_t expected = page_pattern(i, off);
            if (page[off] != expected)
                memtest_fail("neo-PMM page verify",
                             i,
                             off,
                             expected,
                             page[off]);
        }
    }

    for (size_t i = count; i > 0; i--)
    {
        n_frame_free(g_pmm_frames[i - 1]);
        g_pmm_frames[i - 1] = 0;
    }

    size_t after_free = frame_free_count();
    if (after_free != before)
    {
        printf_("%s%llu%s%llu\n",
                "[MEMTEST] PMM free-count leak: before=",
                (unsigned long long) before,
                " after=",
                (unsigned long long) after_free);
        panic("memtest: PMM did not recover all round-trip frames");
    }
}

static const size_t g_basic_sizes[] = {
    1u,    7u,    15u,   16u,   17u,   31u,   32u,
    33u,   63u,   64u,   65u,   255u,  256u,  257u,
    1000u, 4095u, 4096u, 4097u, 8192u, 16384u,
};

static void basic_heap_test(void)
{
    printf_("%s\n", "[MEMTEST] 2/6 basic kmalloc/free boundaries...");
    clear_slots();

    size_t count = sizeof(g_basic_sizes) / sizeof(g_basic_sizes[0]);
    for (size_t i = 0; i < count; i++)
    {
        size_t size = g_basic_sizes[i];
        uint8_t tag = (uint8_t) (0x31u + i);
        set_slot(0, kmalloc(size), size, tag, "basic allocation");
        verify_slot("basic verify", 0);
        release_slot("basic free", 0);
    }
}

static void fragmentation_test(void)
{
    printf_("%s\n", "[MEMTEST] 3/6 fragmentation/split/coalesce...");
    clear_slots();

    for (size_t i = 0; i < MEMTEST_SLOT_COUNT; i++)
    {
        size_t size = 24u + ((i * 173u) % 7000u);
        uint8_t tag = (uint8_t) (0x40u ^ i);
        if (tag == 0)
            tag = 0x40u;
        set_slot(i, kmalloc(size), size, tag, "fragmentation initial fill");
    }

    verify_all("fragmentation initial verify");

    for (size_t i = 1; i < MEMTEST_SLOT_COUNT; i += 2)
        release_slot("fragmentation hole creation", i);

    verify_all("fragmentation survivors");

    for (size_t i = 1; i < MEMTEST_SLOT_COUNT; i += 2)
    {
        size_t size = 73u + ((i * 521u) % 9000u);
        uint8_t tag = (uint8_t) (0xB3u ^ i);
        if (tag == 0)
            tag = 0xB3u;
        set_slot(i, kmalloc(size), size, tag, "fragmentation hole refill");
    }

    verify_all("fragmentation refill verify");

    /* 73 is coprime with 128, so this visits every slot exactly once. */
    for (size_t step = 0; step < MEMTEST_SLOT_COUNT; step++)
    {
        size_t index = (step * 73u) % MEMTEST_SLOT_COUNT;
        release_slot("fragmentation shuffled free", index);
        if ((step & 31u) == 31u)
            schedule();
    }
}

static void realloc_test(void)
{
    printf_("%s\n", "[MEMTEST] 4/6 krealloc preservation...");
    clear_slots();

    static const size_t sizes[] = {
        100u, 200u, 2000u, 4097u, 16384u, 8192u, 128u,
    };

    uint8_t tag = 0x5Au;
    set_slot(0, kmalloc(sizes[0]), sizes[0], tag, "realloc initial");

    for (size_t i = 1; i < sizeof(sizes) / sizeof(sizes[0]); i++)
    {
        verify_slot("realloc precheck", 0);

        size_t old_size = g_slots[0].size;
        uint8_t *old_ptr = g_slots[0].ptr;
        uint8_t *new_ptr = krealloc(old_ptr, sizes[i]);
        if (!new_ptr)
            panic("memtest: krealloc unexpectedly failed");

        size_t preserved = old_size < sizes[i] ? old_size : sizes[i];
        verify_prefix("realloc preserved prefix", 0, new_ptr, preserved, tag);

        g_slots[0].ptr  = new_ptr;
        g_slots[0].size = sizes[i];
        g_slots[0].tag  = tag;
        verify_alignment("realloc alignment", 0, new_ptr);
        fill_slot(0);
    }

    release_slot("realloc chain free", 0);

    uint8_t *special = krealloc(NULL, 512u);
    set_slot(0, special, 512u, 0xC7u, "krealloc NULL");
    verify_slot("krealloc NULL verify", 0);

    uint8_t *zero_result = krealloc(g_slots[0].ptr, 0);
    if (zero_result != NULL)
        panic("memtest: krealloc(ptr, 0) did not return NULL");
    g_slots[0].ptr  = NULL;
    g_slots[0].size = 0;
    g_slots[0].tag  = 0;

    /* Encourage the in-place grow path by freeing the immediate successor. */
    set_slot(0, kmalloc(2048u), 2048u, 0xD1u, "realloc adjacent A");
    set_slot(1, kmalloc(8192u), 8192u, 0xD2u, "realloc adjacent B");
    release_slot("realloc free adjacent B", 1);

    uint8_t *before = g_slots[0].ptr;
    uint8_t *grown  = krealloc(before, 6144u);
    if (!grown)
        panic("memtest: adjacent krealloc failed");
    verify_prefix("realloc adjacent preservation", 0, grown, 2048u, 0xD1u);

    g_slots[0].ptr  = grown;
    g_slots[0].size = 6144u;
    g_slots[0].tag  = 0xD1u;
    fill_slot(0);
    verify_slot("realloc adjacent final verify", 0);

    if (grown == before)
        printf_("%s\n", "[MEMTEST] krealloc in-place growth path observed.");
    else
        printf_("%s\n", "[MEMTEST] krealloc used fallback move path.");

    release_slot("realloc adjacent final free", 0);
}

static void heap_growth_test(void)
{
    printf_("%s\n", "[MEMTEST] 5/6 forcing multi-megabyte heap growth...");
    clear_slots();

    size_t frames_before = frame_free_count();

    for (size_t i = 0; i < MEMTEST_GROWTH_SLOTS; i++)
    {
        size_t size = MEMTEST_GROWTH_SIZE + ((i & 7u) * 257u);
        uint8_t tag = (uint8_t) (0x70u + i);
        set_slot(i, kmalloc(size), size, tag, "heap growth allocation");

        if ((i & 7u) == 7u)
            schedule();
    }

    verify_all("heap growth verify");

    size_t frames_after_alloc = frame_free_count();

    for (size_t i = MEMTEST_GROWTH_SLOTS; i > 0; i--)
        release_slot("heap growth free", i - 1);

    size_t frames_after_free = frame_free_count();

    printf_("%s%llu\n",
            "[MEMTEST] PMM frames before heap growth: ",
            (unsigned long long) frames_before);
    printf_("%s%llu\n",
            "[MEMTEST] PMM frames after heap growth:  ",
            (unsigned long long) frames_after_alloc);
    printf_("%s%llu\n",
            "[MEMTEST] PMM frames after heap frees:   ",
            (unsigned long long) frames_after_free);

    if (frames_after_free != frames_after_alloc)
        panic("memtest: heap frees unexpectedly changed PMM frame ownership");
}

static uint64_t xorshift64(uint64_t *state)
{
    uint64_t x = *state;
    x ^= x << 13;
    x ^= x >> 7;
    x ^= x << 17;
    *state = x;
    return x;
}

static void stats_add_live(memtest_stats_t *stats, size_t amount)
{
    stats->live_bytes += amount;
    if (stats->live_bytes > stats->peak_live_bytes)
        stats->peak_live_bytes = stats->live_bytes;
}

static void random_cycle(uint64_t seed, memtest_stats_t *stats)
{
    clear_slots();

    for (uint64_t op = 0; op < MEMTEST_RANDOM_OPS; op++)
    {
        size_t index =
            (size_t) (xorshift64(&seed) % MEMTEST_SLOT_COUNT);

        if (!g_slots[index].ptr)
        {
            size_t size =
                1u + (size_t) (xorshift64(&seed) % MEMTEST_RANDOM_MAX_ALLOC);
            uint8_t tag = (uint8_t) (xorshift64(&seed) & 0xFFu);
            if (tag == 0)
                tag = 0xA5u;

            set_slot(index,
                     kmalloc(size),
                     size,
                     tag,
                     "random allocation");
            stats->allocations++;
            stats_add_live(stats, size);
        }
        else
        {
            uint64_t action = xorshift64(&seed) % 100u;

            if (action < 35u)
            {
                size_t old_size = g_slots[index].size;
                release_slot("random free", index);
                stats->frees++;
                stats->live_bytes -= old_size;
            }
            else if (action < 75u)
            {
                verify_slot("random realloc precheck", index);

                size_t old_size = g_slots[index].size;
                uint8_t tag     = g_slots[index].tag;
                uint8_t *old_ptr = g_slots[index].ptr;
                size_t new_size =
                    1u + (size_t) (xorshift64(&seed) % MEMTEST_RANDOM_MAX_ALLOC);

                uint8_t *new_ptr = krealloc(old_ptr, new_size);
                if (!new_ptr)
                    panic("memtest: random krealloc failed");

                size_t preserved = old_size < new_size ? old_size : new_size;
                verify_prefix("random realloc preservation",
                              index,
                              new_ptr,
                              preserved,
                              tag);

                g_slots[index].ptr  = new_ptr;
                g_slots[index].size = new_size;
                g_slots[index].tag  = tag;
                verify_alignment("random realloc alignment", index, new_ptr);
                verify_no_overlap("random realloc overlap",
                                  index,
                                  new_ptr,
                                  new_size);
                fill_slot(index);

                stats->reallocations++;
                stats->live_bytes -= old_size;
                stats_add_live(stats, new_size);
            }
            else
            {
                verify_slot("random spot verify", index);
            }
        }

        stats->operations++;

        if ((op & 63u) == 63u)
        {
            verify_all("random periodic full verify");
            schedule();
        }
    }

    verify_all("random final verify");

    for (size_t step = 0; step < MEMTEST_SLOT_COUNT; step++)
    {
        size_t index = (step * 73u) % MEMTEST_SLOT_COUNT;
        if (g_slots[index].ptr)
        {
            size_t old_size = g_slots[index].size;
            release_slot("random cleanup", index);
            stats->frees++;
            stats->live_bytes -= old_size;
        }
    }

    if (stats->live_bytes != 0)
        panic("memtest: random-cycle live byte accounting did not return to zero");
}

static void randomized_heap_test(void)
{
    printf_("%s\n", "[MEMTEST] 6/6 deterministic randomized torture...");

    const uint64_t seed = 0x564F59414745524FULL; /* \"VOYAGERO\" */

    memtest_stats_t first = {0};
    memtest_stats_t second = {0};

    size_t frames_before = frame_free_count();
    random_cycle(seed, &first);
    size_t frames_after_first = frame_free_count();

    random_cycle(seed, &second);
    size_t frames_after_second = frame_free_count();

    printf_("%s%llu\n", "[MEMTEST] Random operations/cycle: ",
            (unsigned long long) first.operations);
    printf_("%s%llu\n", "[MEMTEST] Allocations: ",
            (unsigned long long) first.allocations);
    printf_("%s%llu\n", "[MEMTEST] Frees: ",
            (unsigned long long) first.frees);
    printf_("%s%llu\n", "[MEMTEST] Reallocations: ",
            (unsigned long long) first.reallocations);
    printf_("%s%llu\n", "[MEMTEST] Peak live bytes: ",
            (unsigned long long) first.peak_live_bytes);
    printf_("%s%llu%s%llu%s%llu\n",
            "[MEMTEST] PMM free frames before/after1/after2: ",
            (unsigned long long) frames_before,
            " / ",
            (unsigned long long) frames_after_first,
            " / ",
            (unsigned long long) frames_after_second);

    if (first.operations != second.operations ||
        first.allocations != second.allocations ||
        first.reallocations != second.reallocations ||
        first.peak_live_bytes != second.peak_live_bytes)
        panic("memtest: deterministic random cycles produced different stats");

    if (frames_after_second < frames_after_first)
    {
        printf_("%s\n",
                "[MEMTEST] WARNING: second identical cycle grew the heap again.");
        printf_("%s\n",
                "[MEMTEST] Data integrity passed, but inspect fragmentation/reuse.");
    }
}

void memtest_run(void)
{
    printf_("%s\n", "");
    printf_("%s\n", "============================================");
    printf_("%s\n", " VoyagerOS64 Stage-2 Memory Qualification");
    printf_("%s\n", "============================================");

    pmm_roundtrip_test();
    basic_heap_test();
    fragmentation_test();
    realloc_test();
    heap_growth_test();
    randomized_heap_test();

    clear_slots();

    printf_("%s\n", "============================================");
    printf_("%s\n", "[MEMTEST] PASS - no corruption detected");
    printf_("%s\n", "============================================");
}
