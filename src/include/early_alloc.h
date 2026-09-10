/**
 * Copyright (c) 2025 Tristan Adams
 *
 * This software is released under the MIT License.
 * https://opensource.org/licenses/MIT
 */

/**
 * @file early_alloc.h
 * @brief Bootstrap physical-page allocator used before the permanent PMM.
 * @ingroup early_alloc
 */
#pragma once
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

/**
 * @brief Physical-address type used by the Stage-2 memory stack.
 *
 * A paddr_t is not directly dereferenceable merely because it is represented by
 * an integer. Convert/map it through the HHDM or another explicit mapping
 * before treating it as a pointer.
 */
typedef uint64_t paddr_t;

/**
 * @brief Select a Limine USABLE memory region and initialize the bump
 * allocator.
 *
 * If early_set_region() has already supplied an explicit region, this function
 * preserves that selection rather than choosing another one.
 */
void early_init(void);

/**
 * @brief Allocate one 4 KiB physical page from the bootstrap region.
 * @return 4 KiB-aligned physical address of the allocated page.
 *
 * Early allocations are monotonic and are not individually freed. Once the
 * permanent frame allocator is initialized, paging obtains new frames through
 * the Stage-2 frame supplier instead.
 */
paddr_t early_alloc_page(void);

/**
 * @brief Report the physical range consumed by early allocation.
 * @param base Receives the physical base of the bootstrap region.
 * @param pages Receives the number of pages consumed from that region.
 *
 * The permanent PMM uses this information to avoid handing bootstrap-owned
 * memory back out as free frames.
 */
void early_reserved_region(paddr_t *base, size_t *pages);

/**
 * @brief Return the number of 4 KiB pages consumed by the bootstrap allocator.
 * @return Number of pages allocated since initialization.
 */
size_t early_pages_used(void);

/**
 * @brief Supply an explicit region for early allocation.
 * @param base 4 KiB-aligned physical base address.
 * @param page_count Number of 4 KiB pages in the region.
 *
 * The region must be suitable for allocation and should be configured before
 * early_init().
 */
void early_set_region(paddr_t base, size_t page_count);
