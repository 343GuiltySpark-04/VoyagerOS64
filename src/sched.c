#include "include/sched.h"
#include "include/mm/kmalloc.h"
#include "include/panic.h"
#include "include/string.h"
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define STACK_SIZE (16u * 1024u)
#define STACK_ALIGN 16u

/* This remains false for the cooperative 0.0.5 bring-up. The PIT handler uses
 * it as the gate for IRQ-time preemption, which is deliberately parked until
 * context switching is moved onto a proper interrupt-frame design. */
bool allow_sched = false;
process_t *current = NULL;

static process_t *run_queue_head = NULL;
static process_t *run_queue_tail = NULL;
static int next_pid = 1;

static void task_returned(void)
{
    int pid = current ? current->pid : -1;
    panic("scheduler: task %i returned unexpectedly", pid);

    for (;;)
        asm volatile("cli; hlt");
}

void init_scheduler(void)
{
    current = NULL;
    run_queue_head = NULL;
    run_queue_tail = NULL;
    next_pid = 1;
    allow_sched = false;
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

    p->pid = next_pid++;
    p->state = PROC_READY;

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
    uintptr_t stack_end = (uintptr_t)stack + STACK_SIZE;
    stack_end &= ~((uintptr_t)STACK_ALIGN - 1u);
    stack_end -= sizeof(uint64_t);

    uint64_t *sp = (uint64_t *)stack_end;
    *sp = (uint64_t)(uintptr_t)task_returned;

    *(--sp) = (uint64_t)(uintptr_t)entry; /* ret target */
    *(--sp) = 0; /* rbp */
    *(--sp) = 0; /* rbx */
    *(--sp) = 0; /* r12 */
    *(--sp) = 0; /* r13 */
    *(--sp) = 0; /* r14 */
    *(--sp) = 0; /* r15 */

    p->rsp = sp;

    if (!run_queue_head)
    {
        run_queue_head = p;
        run_queue_tail = p;
        p->next = p;
    }
    else
    {
        p->next = run_queue_head;
        run_queue_tail->next = p;
        run_queue_tail = p;
    }

    return p;
}

void schedule(void)
{
    if (!run_queue_head)
        return;

    process_t *next = current ? current->next : run_queue_head;
    if (!next)
        return;

    /* A single runnable task has nowhere to yield to after first dispatch. */
    if (current && next == current)
        return;

    if (current)
        current->state = PROC_READY;

    next->state = PROC_RUNNING;
    switch_to(next);
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
