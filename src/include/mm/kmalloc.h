/**
 * Copyright (c) 2025 Tristan Adams
 *
 * This software is released under the MIT License.
 * https://opensource.org/licenses/MIT
 */

/**
 * @file kmalloc.h
 * @brief General-purpose Stage-2 kernel heap interface.
 * @ingroup heap
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

    /**
     * @brief Initialize the kernel heap.
     *
     * Must be called after paging_bootstrap() has installed Voyager's page
     * tables and after the permanent physical frame allocator is available.
     * The heap may grow by requesting physical frames and mapping additional
     * virtual pages.
     */
    void kmalloc_init(void);

    /**
     * @brief Allocate at least @p size bytes of kernel virtual memory.
     * @param size Requested payload size in bytes.
     * @return Pointer to allocated kernel memory, or NULL if the request cannot
     *         be satisfied according to heap semantics.
     */
    void *kmalloc(size_t size);

    /**
     * @brief Return a heap allocation to the kernel allocator.
     * @param ptr Pointer previously returned by kmalloc()/krealloc(), or NULL.
     *
     * Freed blocks become reusable by later heap allocations. In the current
     * 0.0.5 design, freeing a block does not shrink the mapped heap high-water
     * mark or immediately return the heap's backing pages to the PMM.
     */
    void kfree(void *ptr);

    /**
     * @brief Resize an existing heap allocation while preserving payload data.
     * @param ptr Existing allocation, or NULL to behave like kmalloc().
     * @param newsize New requested payload size in bytes.
     * @return Resized allocation, which may move, or NULL according to
     * allocator semantics.
     *
     * krealloc(ptr, 0) releases the allocation. Data up to the minimum of the
     * old and new usable sizes is preserved when the allocation moves.
     */
    void *krealloc(void *ptr, size_t newsize);

    /**
     * @brief Query the allocator's usable payload size for an allocation.
     * @param ptr Active heap allocation.
     * @return Number of usable payload bytes associated with @p ptr.
     */
    size_t kmalloc_usable_size(void *ptr);

    /**
     * @brief Emit allocator state for debugging.
     *
     * This is a diagnostic helper and is not part of the allocation fast path.
     */
    void kmalloc_dump(void);

#ifdef __cplusplus
}
#endif
#endif
