/**
 * @file panic.h
 * @brief Fatal kernel error-reporting interface.
 * @ingroup diagnostics
 */
#pragma once
#ifndef _PANIC_H
#define _PANIC_H

/**
 * @brief Report an unrecoverable kernel error and stop normal execution.
 *
 * The first argument is a printf-style format string followed by matching
 * variadic arguments. panic() is the common failure path for violated kernel
 * invariants such as allocator corruption, impossible scheduler state, or
 * missing mandatory boot resources. It does not return.
 *
 * @note Doxygen 1.9.x can misparse the GNU noreturn attribute when attaching
 * parameter documentation to this declaration, so the format-string contract
 * is described in prose rather than with a dedicated parameter tag.
 */
__attribute__((noreturn)) void panic(const char *fmt, ...);

#endif
