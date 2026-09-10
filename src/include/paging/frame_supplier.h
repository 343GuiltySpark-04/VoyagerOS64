/**
 * Copyright (c) 2025 Tristan Adams
 *
 * This software is released under the MIT License.
 * https://opensource.org/licenses/MIT
 */

/**
 * @file frame_supplier.h
 * @brief Boot-stage physical-page supplier indirection used by paging.
 * @ingroup paging
 *
 * Paging must allocate page-table frames both before and after the permanent
 * PMM is initialized. This tiny adapter keeps paging independent of the
 * allocator transition: early boot points it at early_alloc_page(), then memory
 * bring-up switches it to frame_alloc().
 */
#pragma once
#ifndef FRAME_SUPPLIER_H
#define FRAME_SUPPLIER_H

#include <stdint.h>

/** @brief Function type returning one physical page address. */
typedef uint64_t (*phys_page_supplier_t)(void);

/**
 * @brief Active boot-time physical-page supplier.
 *
 * Kernel bring-up owns this pointer and changes it exactly when allocator
 * authority moves from the early bump allocator to the permanent PMM.
 */
extern phys_page_supplier_t g_boot_phys_page;

/**
 * @brief Request one page-table frame from the currently authoritative
 * supplier.
 * @return Physical page address, or 0 if no supplier has been installed.
 */
static inline uint64_t PagingGetFreeFrame(void)
{
    return g_boot_phys_page ? g_boot_phys_page() : 0;
}

#endif
