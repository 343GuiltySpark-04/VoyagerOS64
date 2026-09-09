#pragma once
#ifndef _KERNEL_UTILS_H
#define _KERNEL_UTILS_H

#include "global_defs.h"
#include <stdint.h>

struct PACKED kswitches
{
    uint8_t stack_trace_size;
    uint8_t stack_trace_on_fault;
    uint8_t acpi_support;
    uint8_t sched_debug;
    uint8_t addr_debug;
    uint8_t hw_rng_support;
    uint8_t mem_readout_unit;
    uint8_t liballoc_debug;
    uint8_t fpu_allowed;
    uint8_t timestamp;
};

extern const struct kswitches k_mode;

void print_memmap(void);
uint64_t get_memory_size(void);
uint64_t get_memory_size_gib(void);
uint64_t bytes_to_mib(uint64_t bytes);
uint64_t bytes_to_gib(uint64_t bytes);

struct term_context;
extern struct term_context *term_context;

extern void restore_floats(void);
extern void save_floats(void);

#endif
