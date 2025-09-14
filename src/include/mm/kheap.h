/**
 * Copyright (c) 2025 Tristan Adams
 *
 * This software is released under the MIT License.
 * https://opensource.org/licenses/MIT
 */

#pragma once
#ifndef KHEAP_H
#define KHEAP_H

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C"
{
#endif

    /* Initialize the kernel heap VA window; no pages mapped yet. */
    void kheap_init(void);

    /* Optional: pre-reserve N pages upfront (maps VA, allocs frames). */
    void kheap_reserve_pages(size_t pages);

#ifdef __cplusplus
}
#endif
#endif
