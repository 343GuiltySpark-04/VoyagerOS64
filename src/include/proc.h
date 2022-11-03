#pragma once

#ifndef _PROC_H
#define _PROC_H
#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include "lock.h"
#include "lib/vector.h"
#include "registers.h"
#include "cpuUtils.h"
#include "global_defs.h"

#define MAX_FDS 256

struct process
{

    int pid;
    int ppid;
    int status;
    struct pagemap *pagemap;
    uintptr_t mmap_anon_base;
    uintptr_t stack_top;
    VECTOR_TYPE(struct thread *)
    threads;
    VECTOR_TYPE(struct process *)
    children;
    mode_t umask;
    char name[128];
};

struct thread
{

    struct thread *self;
    uint64_t errno;

    spinlock_t lock;
    struct cpu_local *this_cpu;
    int running_on;
    bool enqueued;
    bool enqueued_by_signal;
    struct process *process;
    int timeslice;
    spinlock_t yield_await;
    struct cpu_ctx ctx;
    void *gs_base;
    void *fs_base;
    uint64_t cr3;
    void *fpu_bank_size;
    VECTOR_TYPE(void *)
    stacks;
    void *pf_stack;
    void *kernel_stack;
};

static inline struct thread *sched_current_thread(void)
{
    struct thread *ret = NULL;
    asm volatile("mov %%gs:0x0, %0"
                 : "=r"(ret));
    return ret;
}

#endif