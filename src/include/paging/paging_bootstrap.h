/**
 * Copyright (c) 2025 Tristan Adams
 *
 * This software is released under the MIT License.
 * https://opensource.org/licenses/MIT
 */

#pragma once
#include <stdint.h>

#ifdef __cplusplus
extern "C"
{
#endif

    // Build a fresh kernel page table and install it in CR3.
    // Assumes g_boot_phys_page is set to early_alloc_page() BEFORE calling.
    // After this, you can run frame_init() and flip to frame_alloc().
    void paging_bootstrap(void);

#ifdef __cplusplus
}
#endif
