/**
 * Copyright (c) 2026 Tristan Adams
 *
 * VoyagerOS64 memory qualification harness.
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
 * Intended to be invoked after memory_bringup(), kmalloc_init(), and scheduler
 * startup. The test is destructive only to memory it allocates itself.
 */
void memtest_run(void);

#ifdef __cplusplus
}
#endif

#endif
