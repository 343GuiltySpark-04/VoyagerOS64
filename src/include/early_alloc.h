/**
 * Copyright (c) 2025 Tristan Adams
 *
 * This software is released under the MIT License.
 * https://opensource.org/licenses/MIT
 */

// early_alloc.h
#pragma once
#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

// Physical address type
typedef uint64_t paddr_t;

// Public API
void early_init(void);                                    // choose a USABLE region and arm the bump ptr
paddr_t early_alloc_page(void);                           // returns a 4KiB-aligned physical frame
void early_reserved_region(paddr_t *base, size_t *pages); // report what we consumed
size_t early_pages_used(void);

// Optional: reserve a specific region for early alloc (must be USABLE and page-aligned).
// If you call this before early_init(), early_init() will be a no-op.
void early_set_region(paddr_t base, size_t page_count);
