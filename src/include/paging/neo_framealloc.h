/**
 * Copyright (c) 2025 Tristan Adams
 *
 * This software is released under the MIT License.
 * https://opensource.org/licenses/MIT
 */

/**
 * @file neo_framealloc.h
 * @brief Permanent Stage-2 physical frame allocator interface.
 * @ingroup pmm
 */
#pragma once
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "../early_alloc.h"

/**
 * @brief Initialize the permanent physical memory manager from Limine's memmap.
 *
 * Builds allocator metadata from usable regions and reserves ranges already
 * consumed by the kernel, boot environment, and early allocator. Call after
 * paging_bootstrap() while the bootstrap allocator's ownership information is
 * still available.
 */
void frame_init(void);

/**
 * @brief Allocate one 4 KiB physical frame.
 * @return Physical address of an allocated 4 KiB frame.
 *
 * The returned value is a physical address, not a C pointer. The allocator
 * panics on exhaustion rather than returning an ordinary heap-style NULL.
 */
paddr_t frame_alloc(void);

/**
 * @brief Return a 4 KiB physical frame to the PMM.
 * @param paddr Physical address previously obtained from frame_alloc().
 *
 * The implementation guards alignment, allocator bounds, non-usable regions,
 * and double-free attempts.
 */
void n_frame_free(paddr_t paddr);

/**
 * @brief Count currently free physical frames.
 * @return Number of allocatable 4 KiB frames currently free.
 */
size_t frame_free_count(void);

/**
 * @brief Count physical frames managed by the PMM.
 * @return Number of 4 KiB frames represented by the active allocator.
 */
size_t frame_total_count(void);

/**
 * @brief Query allocation state for a physical frame.
 * @param paddr 4 KiB-aligned physical frame address.
 * @return true when the frame is currently marked allocated; false otherwise.
 */
bool frame_is_allocated(paddr_t paddr);
