/**
 * @file stack_trace.h
 * @brief Stack-size reporting, frame walking, and diagnostic stack dumps.
 * @ingroup diagnostics
 */
#pragma once

#ifndef _STACK_TRACE_H
#define _STACK_TRACE_H
#include "global_defs.h"
#include "limine.h"
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

/** Limine request used to learn the boot-provided kernel stack size. */
extern volatile struct limine_stack_size_request stack_req;

/** @brief Collected stack-return-address array and its populated length. */
struct stack_return
{
    uint64_t *addresses;
    uint64_t  array_size;
};

/** @brief Frame-pointer chain node used by the C stack walker. */
struct stack_frame
{
    struct stack_frame *rbp;
    uint64_t            rip;
};

/**
 * @brief Assembly-assisted stack trace entry point.
 * @param max_size Maximum number/size of entries requested by the
 * implementation.
 * @param stop Whether the helper should stop after producing the trace.
 */
void stack_trace_asm(uint64_t max_size, bool stop);

/** @brief Print the kernel stack size reported by Limine. */
void print_stack_size(void);

/**
 * @brief Walk and print up to @p max_frames frame-pointer entries.
 * @param max_frames Maximum number of frames to report.
 */
void stack_trace(uint64_t max_frames);

/**
 * @brief Emit a hexadecimal dump of an arbitrary byte region.
 * @param data Start of the region to print.
 * @param size Number of bytes to dump.
 */
void dump_hex(const void *data, size_t size);

/**
 * @brief Low-level stack-dump assembly helper.
 * @param leave Historical leave/return behavior selector.
 * @param trace_size Requested trace extent.
 */
extern void stack_dump_asm(uint64_t leave, uint64_t trace_size);

/** @brief Emit the default diagnostic stack dump. */
void stack_dump(void);

/**
 * @brief Emit a recursive/frame-oriented stack dump.
 * @param max_frames Maximum number of frames to inspect.
 */
void stack_dump_recursive(uint64_t max_frames);

#endif
