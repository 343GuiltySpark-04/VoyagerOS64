/**
 * Copyright (c) 2025 Tristan Adams
 *
 * This software is released under the MIT License.
 * https://opensource.org/licenses/MIT
 */

// framealloc.h
#pragma once
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "../early_alloc.h" // for paddr_t

void frame_init(void); // build bitmaps from Limine memmap, mark reserved ranges
paddr_t frame_alloc(void); // physical frame (4KiB), panics on OOM
void    n_frame_free(
       paddr_t paddr); // guards: alignment, bounds, double-free, non-usable
size_t frame_free_count(void);
size_t frame_total_count(void);

// Optional helpers
bool frame_is_allocated(paddr_t paddr);
