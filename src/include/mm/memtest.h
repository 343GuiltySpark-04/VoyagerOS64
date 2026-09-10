/**
 * Copyright (c) 2026 Tristan Adams
 *
 * VoyagerOS64 memory qualification harness.
 */

/**
 * @file memtest.h
 * @brief Stage-2 PMM/heap qualification harness.
 * @ingroup diagnostics
 */
#pragma once
#ifndef VOYAGER_MEMTEST_H
#define VOYAGER_MEMTEST_H

#ifdef __cplusplus
extern "C"
{
#endif

/**
 * @brief Run the Stage-2 PMM/heap qualification suite.
 *
 * Intended to be invoked after paging bootstrap, frame allocator initialization,
 * kmalloc initialization, and scheduler startup. The test is destructive only
 * to memory it allocates itself and yields periodically during long randomized
 * phases so the cooperative scheduler continues to run.
 *
 * The suite exercises direct frame allocation/free, heap boundary sizes,
 * fragmentation and coalescing, krealloc preservation, multi-megabyte heap
 * growth, and deterministic randomized allocation/free/reallocation cycles.
 *
 * @note Current heap pages are retained after kfree(). A lower PMM free-frame
 * count after the first high-water workload is therefore expected; repeated
 * identical workloads should plateau once the heap has grown sufficiently.
 *
 * @warning A detected corruption or violated allocator invariant is treated as
 * a kernel qualification failure and may panic the system.
 */
void memtest_run(void);

#ifdef __cplusplus
}
#endif

#endif
