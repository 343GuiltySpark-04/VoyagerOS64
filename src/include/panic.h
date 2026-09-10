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
 * @param fmt printf-style format string followed by matching arguments.
 *
 * panic() is the common failure path for violated kernel invariants such as
 * allocator corruption, impossible scheduler state, or missing mandatory boot
 * resources. It does not return.
 */
__attribute__((noreturn)) void panic(const char *fmt, ...);

#endif
