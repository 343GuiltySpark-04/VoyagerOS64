/**
 * Copyright (c) 2025 Tristan Adams
 *
 * This software is released under the MIT License.
 * https://opensource.org/licenses/MIT
 */

/**
 * @file paging_bootstrap.h
 * @brief Construction and installation of Voyager-owned bootstrap page tables.
 * @ingroup paging
 */
#pragma once
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C"
{
#endif

    /**
     * @brief Build VoyagerOS64's initial kernel page-table hierarchy and install
     *        it in CR3.
     *
     * Page-table pages are supplied from early_alloc_page() during bootstrap.
     * The resulting mappings preserve the kernel image, active stack, HHDM, and
     * other ranges required for the post-Limine kernel to continue executing.
     *
     * @warning After the new CR3 is installed, code must not assume Limine-owned
     * callback code remains mapped or callable. Voyager's framebuffer/serial
     * facilities own output after the handoff.
     */
    void paging_bootstrap(void);

    /**
     * @brief Return the physical address loaded as the bootstrap PML4 root.
     * @return Physical address of Voyager's root page table.
     */
    uint64_t paging_pml4_phys(void);

    /**
     * @brief Return a kernel-accessible virtual pointer to the bootstrap PML4.
     * @return Virtual address through which the root page table can be accessed.
     */
    void *paging_pml4_virt(void);

#ifdef __cplusplus
}
#endif
