#include <stdint.h>
#include "include/printf.h"
#include "include/liballoc.h"
#include "include/sched.h"
#include "include/lock.h"
#include "include/gdt.h"
#include "include/pic.h"
#include "include/string.h"
#include "include/idt.h"
#include <stdbool.h>
#include "include/stack_trace.h"
#include "include/KernelUtils.h"
#include "include/kernel.h"
#include "include/memUtils.h"
#include "include/panic.h"
#include "include/serial.h"

#define SAVE_STATE()                       \
    asm volatile("pushq %rax");            \
    asm volatile("pushq %rcx");            \
    asm volatile("pushq %rdx");            \
    asm volatile("pushq %rbx");            \
    asm volatile("pushq %rsp");            \
    asm volatile("pushq %rbp");            \
    asm volatile("pushq %rsi");            \
    asm volatile("pushq %rdi");            \
    asm volatile("pushq %r8");             \
    asm volatile("pushq %r9");             \
    asm volatile("pushq %r10");            \
    asm volatile("pushq %r11");            \
    asm volatile("pushq %r12");            \
    asm volatile("pushq %r13");            \
    asm volatile("pushq %r14");            \
    asm volatile("pushq %r15");            \
    asm volatile("movq %cr3, %rax");       \
    asm volatile("movq %rax, %cr3");       \
    asm volatile("movq %cr0, %rax");       \
    asm volatile("orq $0x80000000, %rax"); \
    asm volatile("movq %rax, %cr0");       \
    asm volatile("ltr %ax"                 \
                 :                         \
                 : "a"(TSS_SELECTOR));

#define STACK_SIZE 4096

// bool timer_fired = false;

bool sched_started = false;

static spinlock_t schedlock_t = SPINLOCK_INIT;

const uint64_t quantum = 5;

const uint64_t quantum_limit = 15;

uint64_t active_pid;

uint64_t exec_pid;

extern breakpoint();

extern halt();

static char sched_buff[64];

static int next_pid = 1;
process_t *current = 0;

bool allow_sched = false;

void init_scheduler(void)
{
    current = 0; // nothing yet

    allow_sched = true;
}

process_t *create_process(void (*entry)(void))
{

    // spinlock_acquire(&schedlock_t);

    process_t *p = ALLOC(sizeof(process_t));
    p->pid = next_pid++;
    p->state = PROC_READY;

    uint64_t *stack = ALLOC(STACK_SIZE);
    uint64_t *sp = (uint64_t *)((uint8_t *)stack + STACK_SIZE);

    // push fake callee-saved registers (switch frame layout)
    *(--sp) = (uint64_t)0; // r15
    *(--sp) = (uint64_t)0; // r14
    *(--sp) = (uint64_t)0; // r13
    *(--sp) = (uint64_t)0; // r12
    *(--sp) = (uint64_t)0; // rbx
    *(--sp) = (uint64_t)0; // rbp

    // return address for ret in switch_to
    *(--sp) = (uint64_t)entry;

    p->rsp = sp;

    // add to circular list
    if (!current)
    {
        current = p;
        p->next = p;
    }
    else
    {
        p->next = current->next;
        current->next = p;
    }

    // spinlock_release(&schedlock_t);

    return p;
}

void schedule(void)
{

    // spinlock_acquire(&schedlock_t);

    printf_("%s", "PID: ");
    printf_("%i\n", &current->pid);

    if (!current || !current->next)
    {
        // spinlock_release(&schedlock_t);
        return;
    }
    process_t *next = current->next;
    if (next == current)
    {
        // spinlock_release(&schedlock_t);
        return; // only one process
    }
    // spinlock_release(&schedlock_t);
    switch_to(next);
}
