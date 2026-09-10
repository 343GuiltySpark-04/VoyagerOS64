/**
 * @file vmm.h
 * @brief Retired virtual-memory-manager experiment.
 * @ingroup deprecated_code
 * @deprecated The source implementing this interface is excluded from the
 *             active 0.0.5 kernel. Current address-space work uses the paging
 *             interfaces directly; per-process VMM design is future work.
 */
#pragma once
#include <stdint.h>

void VMM_table_clone();

struct dummy_proc
{
    uint64_t id;
    uint64_t data;
};
