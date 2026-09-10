/**
 * @file sched.h
 * @brief Cooperative process scheduler interface.
 * @ingroup scheduler
 *
 * VoyagerOS64 0.0.5 deliberately uses cooperative round-robin scheduling.
 * Tasks run until they call schedule(). IRQ/PIT-driven preemption is not part
 * of the qualified 0.0.5 scheduler path.
 */
#pragma once

#ifndef _SCHED_H
#define _SCHED_H

#include "global_defs.h"
#include <stdbool.h>
#include <stdint.h>

/** Reserved software-syscall identifiers. No fork/exit implementation is
 * provided by the 0.0.5 scheduler merely because these values exist. */
#define SYSCALL_YIELD 1
#define SYSCALL_FORK 2
#define SYSCALL_EXIT 3

/** @brief Scheduler-visible process state. */
typedef enum
{
    /** Runnable but not currently executing. */
    PROC_READY,
    /** Currently selected process. */
    PROC_RUNNING,
    /** Not runnable until some future wake-up mechanism makes it ready. */
    PROC_BLOCKED

} proc_state_t;

/**
 * @brief Minimal 0.0.5 process control block.
 *
 * Processes currently share the kernel address space. Each process owns a
 * separately allocated kernel stack; @c rsp records the saved cooperative
 * context for switch_to(). The @c next pointer forms the circular run queue.
 *
 * @warning sched_ll.asm depends on the layout of this structure, in particular
 *          `rsp` being at byte offset 8. Keep the assembly and C definition in
 *          sync if fields are reordered or resized.
 */
typedef struct process
{
    /** Monotonically assigned kernel process identifier. */
    int pid;

    /** Saved stack pointer consumed/produced by switch_to(). */
    uint64_t *rsp;

    /** Current scheduler state. */
    proc_state_t state;

    /** Next process in the circular run queue. */
    struct process *next;
} process_t;

/**
 * @brief Gate used by the PIT path for scheduler preemption.
 *
 * This remains false in the qualified 0.0.5 cooperative design. Setting it to
 * true does not by itself make switch_to() safe for IRQ-time preemption.
 */
extern bool allow_sched;

/** @brief Process whose stack/context is currently executing, or NULL before
 * the first scheduler dispatch. */
extern process_t *current;

/**
 * @brief Switch cooperatively from the current process to @p next.
 * @param next Process whose saved stack context should be restored.
 *
 * The low-level implementation saves/restores the SysV AMD64 callee-saved
 * registers used by the cooperative C-call boundary. It is not an interrupt
 * frame switch and must not be treated as one.
 */
void switch_to(process_t *next);

/**
 * @brief Create a kernel process and append it to the round-robin run queue.
 * @param entry `void(void)` entry point for the new process.
 * @return Pointer to the new process control block.
 *
 * A 16 KiB stack is allocated from kmalloc and initialized with a synthetic
 * context suitable for switch_to(). A task that returns from @p entry is
 * treated as a kernel error in 0.0.5 because task exit/reaping is not
 * implemented yet.
 */
process_t *create_process(void (*entry)(void));

/**
 * @brief Yield the CPU cooperatively to the next process in the circular queue.
 *
 * Calling schedule() is the 0.0.5 task-yield primitive. With zero runnable
 * tasks it is a no-op; with only the current task runnable it returns without
 * switching.
 */
void schedule(void);

/**
 * @brief Reset scheduler bookkeeping to its pre-dispatch state.
 *
 * Call once during kernel bring-up before creating the initial tasks.
 */
void init_scheduler(void);

/**
 * @brief Begin execution of the initialized run queue.
 *
 * The run queue must contain at least one process and @ref current must still
 * be NULL. This function performs the initial dispatch and is not expected to
 * return.
 */
void scheduler_start(void);

#endif
