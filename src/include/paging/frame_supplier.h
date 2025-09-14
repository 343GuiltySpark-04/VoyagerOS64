/**
 * Copyright (c) 2025 Tristan Adams
 *
 * This software is released under the MIT License.
 * https://opensource.org/licenses/MIT
 */

#pragma once
#ifndef FRAME_SUPPLIER_H
#define FRAME_SUPPLIER_H

#include <stdint.h>

typedef uint64_t (*phys_page_supplier_t)(void);

// Global thunk: assigned during boot (_start)
extern phys_page_supplier_t g_boot_phys_page;

// Paging calls this instead of touching allocators directly
static inline uint64_t PagingGetFreeFrame(void)
{
    return g_boot_phys_page ? g_boot_phys_page() : 0;
}

#endif
