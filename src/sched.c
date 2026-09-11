#include "include/mm/kmalloc.h"
#include "include/panic.h"
#include "include/sched.h"
#include "include/string.h"
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define STACK_SIZE (16u * 1024u)
#define STACK_ALIGN 16u

extern void halt();

/* This remains false for the cooperative 0.0.5 bring-up. The PIT handler uses
 * it as the gate for IRQ-time preemption, which is deliberately parked until
 * context switching is moved onto a proper interrupt-frame design. */
bool       allow_sched = false;
process_t *current     = NULL;

static process_t *run_queue_head = NULL;
static process_t *run_queue_tail = NULL;
static process_t *reaping        = NULL;
static int        next_pid       = 1;

static void task_returned(void)
{
    process_exit(0);
}

void init_scheduler(void)
{
    current        = NULL;
    run_queue_head = NULL;
    run_queue_tail = NULL;
    reaping        = NULL;
    next_pid       = 1;
    allow_sched    = false;
}

process_t *create_process(void (*entry)(void))
{
    if (!entry)
        panic("scheduler: NULL task entry");

    process_t *p = kmalloc(sizeof(*p));
    if (!p)
        panic("scheduler: failed to allocate process structure");

    memset(p, 0, sizeof(*p));

    uint8_t *stack = kmalloc(STACK_SIZE);
    if (!stack)
        panic("scheduler: failed to allocate task stack");

    memset(stack, 0, STACK_SIZE);

    p->pid   = next_pid++;
    p->state = PROC_READY;

    p->stack_base  = stack;
    p->exit_status = 0;

    /*
     * switch_to() restores this exact layout:
     *
     *   rsp -> r15
     *          r14
     *          r13
     *          r12
     *          rbx
     *          rbp
     *          initial RIP (consumed by ret)
     *          task_returned (return address seen by task entry)
     *
     * The task-visible RSP is 8 mod 16, matching the SysV AMD64 function
     * entry convention.
     */
    uintptr_t stack_end = (uintptr_t) stack + STACK_SIZE;
    stack_end &= ~((uintptr_t) STACK_ALIGN - 1u);
    stack_end -= sizeof(uint64_t);

    uint64_t *sp = (uint64_t *) stack_end;
    *sp          = (uint64_t) (uintptr_t) task_returned;

    *(--sp) = (uint64_t) (uintptr_t) entry; /* ret target */
    *(--sp) = 0;                            /* rbp */
    *(--sp) = 0;                            /* rbx */
    *(--sp) = 0;                            /* r12 */
    *(--sp) = 0;                            /* r13 */
    *(--sp) = 0;                            /* r14 */
    *(--sp) = 0;                            /* r15 */

    p->rsp = sp;

    if (!run_queue_head)
    {
        run_queue_head = p;
        run_queue_tail = p;
        p->next        = p;
    }
    else
    {
        p->next              = run_queue_head;
        run_queue_tail->next = p;
        run_queue_tail       = p;
    }

    return p;
}

void process_exit(int status)
{
    if (!current)
        panic("scheduler: process_exit called without a current process");

    process_t *exiting = current;

    exiting->exit_status = status;
    exiting->state       = PROC_EXITED;

    schedule();

    /*
     * An exited process must never resume. If schedule() returns here,
     * scheduler invariants have been violated.
     */
    panic("scheduler: exited task %i resumed", exiting->pid);
}

static process_t *find_next_ready(process_t *start)
{
    if (!start)
        return NULL;

    process_t *candidate = start;

    do
    {
        if (candidate->state == PROC_READY)
            return candidate;

        candidate = candidate->next;
    } while (candidate && candidate != start);

    return NULL;
}

static void tribute_pull(void)
{
    process_t *dead = reaping;

    if (!dead)
        return;

    /*
     * Never free the process whose stack we are currently executing on.
     */
    if (dead == current)
        return;

    reaping = NULL;

    if (dead->stack_base)
    {
        kfree(dead->stack_base);
        dead->stack_base = NULL;
    }

    kfree(dead);
}

static void unlink_process(process_t *tribute)
{
    if (!tribute || !run_queue_head)
        panic("scheduler: invalid process unlink");

    /*
     * One-element circular queue.
     */
    if (tribute->next == tribute)
    {
        run_queue_head = NULL;
        run_queue_tail = NULL;
        tribute->next  = NULL;
        return;
    }

    process_t *previous = run_queue_head;

    do
    {
        if (previous->next == tribute)
            break;

        previous = previous->next;
    } while (previous != run_queue_head);

    if (previous->next != tribute)
        panic("scheduler: process not found in run queue");

    previous->next = tribute->next;

    if (run_queue_head == tribute)
        run_queue_head = tribute->next;

    if (run_queue_tail == tribute)
        run_queue_tail = previous;

    tribute->next = NULL;
}

void schedule(void)
{
    /*
     * Anything left behind by the previous context switch can now be
     * reclaimed safely because we are executing on the current stack.
     */
    tribute_pull();

    if (!run_queue_head)
        return;

    process_t *previous = current;
    process_t *start;

    if (previous && previous->state == PROC_EXITED)
    {
        /*
         * Preserve where traversal should continue before removing the
         * exiting process from the circular queue.
         */
        start = previous->next;

        unlink_process(previous);

        /*
         * reap_deferred() above should always have emptied this slot.
         */
        if (reaping)
            panic("scheduler: deferred reap slot still occupied");

        reaping = previous;
    }
    else
    {
        /*
         * An ordinary cooperative yield makes the running process ready
         * again. BLOCKED and EXITED processes must not be revived.
         */
        if (previous && previous->state == PROC_RUNNING)
            previous->state = PROC_READY;

        start = previous ? previous->next : run_queue_head;
    }

    /*
     * Eventually an idle task will make this case non-fatal. For now,
     * exiting the final runnable process is still a scheduler error.
     */
    if (!run_queue_head)
        panic("scheduler: no processes remain after exit");

    if (!start)
        start = run_queue_head;

    process_t *next = find_next_ready(start);

    if (!next)
        panic("scheduler: no runnable process");

    if (next == previous)
    {
        previous->state = PROC_RUNNING;
        return;
    }

    next->state = PROC_RUNNING;
    switch_to(next);

    /*
     * When switch_to() eventually returns here, we may be the process
     * that another task switched to while exiting. Reap it immediately.
     */
    tribute_pull();
}

void scheduler_start(void)
{
    if (!run_queue_head)
        panic("scheduler: cannot start with an empty run queue");
    if (current)
        panic("scheduler: start requested while a task is already current");

    /* IRQ-time preemption intentionally remains disabled. Tasks yield by
     * calling schedule() from ordinary C context. */
    allow_sched = false;
    schedule();

    panic("scheduler: initial dispatch returned unexpectedly");
}
