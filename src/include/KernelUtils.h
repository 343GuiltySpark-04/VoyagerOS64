/**
 * @file KernelUtils.h
 * @brief Kernel feature switches, memory-reporting helpers, and shared
 * utilities.
 * @ingroup boot
 */
#pragma once
#ifndef _KERNEL_UTILS_H
#define _KERNEL_UTILS_H

#include "global_defs.h"
#include <stdint.h>

/**
 * @brief Compile-time/runtime-style kernel behavior switches.
 *
 * The current tree exposes one constant @ref k_mode instance used by bring-up
 * and diagnostics to enable or suppress optional behavior. These fields are not
 * a stable userspace ABI; they are internal kernel policy/debug controls.
 */
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

/** Active kernel behavior/debug configuration. */
extern const struct kswitches k_mode;

/** @brief Print the Limine-provided physical memory map for diagnostics. */
void print_memmap(void);
/** @brief Return detected physical memory size in the implementation's base
 * unit. */
uint64_t get_memory_size(void);
/** @brief Return detected memory size expressed in GiB. */
uint64_t get_memory_size_gib(void);
/** @brief Convert a byte count to MiB. */
uint64_t bytes_to_mib(uint64_t bytes);
/** @brief Convert a byte count to GiB. */
uint64_t bytes_to_gib(uint64_t bytes);

struct term_context;
/** Voyager-owned terminal context after framebuffer handoff. */
extern struct term_context *term_context;

/**
 * @brief Experimental floating-point restore helper.
 * @warning Per-task floating-point state is not qualified in 0.0.5.
 */
extern void restore_floats(void);
/**
 * @brief Experimental floating-point save helper.
 * @warning Per-task floating-point state is not qualified in 0.0.5.
 */
extern void save_floats(void);

#endif
