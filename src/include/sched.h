/**
 * @file sched.h
 * @brief Cooperative process scheduler interface.
 * @ingroup scheduler
 *
 * VoyagerOS64 currently uses cooperative round-robin scheduling. Tasks run
 * until they call schedule(), block, or exit. IRQ/PIT-driven preemption is not
 * part of the qualified scheduler path yet.
 */
#pragma once

#ifndef _SCHED_H
#define _SCHED_H

#include "global_defs.h"
#include <stdbool.h>
#include <stdint.h>

/** Reserved software-syscall identifiers. */
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
    PROC_BLOCKED,
    /** Execution has finished; the process is awaiting reclamation. */
    PROC_EXITED

} proc_state_t;

/**
 * @brief Minimal cooperative process control block.
 *
 * Processes currently share the kernel address space. Each process owns a
 * separately allocated kernel stack; @c rsp records the saved cooperative
 * context for switch_to(). The @c next pointer forms the circular run queue.
 * @c stack_base preserves the original heap allocation so the scheduler can
 * reclaim a dead task only after switching away from its stack.
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

    /** Base of the heap allocation containing this process's kernel stack. */
    void *stack_base;

    /** Status supplied when this process terminates. */
    int exit_status;

} process_t;

/**
 * @brief Gate used by the PIT path for scheduler preemption.
 *
 * This remains false in the qualified cooperative design. Setting it to true
 * does not by itself make switch_to() safe for IRQ-time preemption.
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
 * context suitable for switch_to(). Returning from @p entry terminates the
 * process with status zero.
 */
process_t *create_process(void (*entry)(void));

/**
 * @brief Yield the CPU cooperatively to the next runnable process.
 *
 * READY processes participate in round-robin selection. BLOCKED processes are
 * skipped until explicitly woken, and EXITED processes are removed and
 * reclaimed after the scheduler has switched away from their stacks.
 */
void schedule(void);

/**
 * @brief Reset scheduler bookkeeping and create the permanent idle process.
 *
 * Call once during kernel bring-up before creating ordinary kernel tasks.
 */
void init_scheduler(void);

/**
 * @brief Begin execution of the initialized run queue.
 *
 * The run queue must contain at least the idle process and @ref current must
 * still be NULL. This function performs the initial dispatch and is not
 * expected to return.
 */
void scheduler_start(void);

/**
 * @brief Terminate the currently running process.
 * @param status Exit status retained until the process is reclaimed.
 *
 * Marks the current process as exited and transfers execution to another
 * runnable process. This function must not return to the exiting task.
 */
void process_exit(int status);

/**
 * @brief Block the currently running process.
 *
 * The process remains in the scheduler queue but is skipped until another
 * context marks it ready again. Returns only after the process has been woken
 * and scheduled again.
 */
void process_block(void);

/**
 * @brief Make a blocked process runnable.
 * @param process Valid process control block to wake.
 * @return true if the process transitioned from BLOCKED to READY.
 *
 * Waking a process does not immediately yield or switch context; it merely
 * makes that process eligible for a future scheduler selection.
 */
bool process_wake(process_t *process);

#endif
