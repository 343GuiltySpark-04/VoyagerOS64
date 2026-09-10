/**
 * @file registers.h
 * @brief Low-level control-register, stack-pointer, and MSR access helpers.
 * @ingroup boot
 */
#pragma once
#include <stdint.h>

/** @brief Read CR0. */
extern uint64_t readCRO(void);
/** @brief Read CR3, the active page-table-root physical address. */
extern uint64_t readCR3(void);
/** @brief Read CR2, commonly used to obtain the page-fault linear address. */
extern uint64_t readCR2(void);
/** @brief Read the current stack pointer. */
extern uint64_t readRSP(void);
/** @brief Read CR4. */
extern uint64_t readCR4(void);

/** @brief Write CR4. */
extern void writeCR4(uint64_t value);
/** @brief Write CR0. */
extern void writeCR0(uint64_t value);
/**
 * @brief Replace the active CR3 value.
 * @param value Physical address/value to load into CR3.
 *
 * @warning Loading CR3 changes the address space immediately. The caller must
 * ensure the executing code, active stack, required descriptor/interrupt state,
 * and subsequent data references remain mapped by the new tables.
 */
extern void writeCR3(uint64_t value);

/**
 * @brief Write a 64-bit model-specific register.
 * @param msr MSR index.
 * @param value Value to write.
 */
void writeMSR(uint64_t msr, uint64_t value);

/**
 * @brief Read a 64-bit model-specific register.
 * @param msr MSR index.
 * @return Current 64-bit MSR value.
 */
uint64_t rdmsr(uint32_t msr);
