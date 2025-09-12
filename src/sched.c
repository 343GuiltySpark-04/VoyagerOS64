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

//bool timer_fired = false;

bool sched_started = false;

static spinlock_t schedlock_t = SPINLOCK_INIT;

const uint64_t quantum = 5;

const uint64_t quantum_limit = 15;

uint64_t active_pid;

uint64_t exec_pid;

extern breakpoint();

extern halt();

