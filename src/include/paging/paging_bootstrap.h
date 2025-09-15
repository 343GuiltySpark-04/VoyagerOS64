/**
 * Copyright (c) 2025 Tristan Adams
 *
 * This software is released under the MIT License.
 * https://opensource.org/licenses/MIT
 */

#pragma once
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C"
{
#endif

    /* Build a new kernel page table and install it in CR3.
       Uses early_alloc_page() internally for all page-table pages. */
    void paging_bootstrap(void);

    /* Optional accessors if you want to use them later */
    uint64_t paging_pml4_phys(void);
    void    *paging_pml4_virt(void);

#ifdef __cplusplus
}
#endif
