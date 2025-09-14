/**
 * Copyright (c) 2025 Tristan Adams
 *
 * This software is released under the MIT License.
 * https://opensource.org/licenses/MIT
 */

#pragma once
#ifndef KMALLOC_H
#define KMALLOC_H

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C"
{
#endif

    void  kmalloc_init(void); // must be called after memory_bringup()
    void *kmalloc(size_t size);
    void  kfree(void *ptr);
    void *krealloc(void *ptr, size_t newsize);

    // Optional helpers
    size_t kmalloc_usable_size(void *ptr);
    void   kmalloc_dump(void); // debug print (optional; no-op if you wish)

#ifdef __cplusplus
}
#endif
#endif
