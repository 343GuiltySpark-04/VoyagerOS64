#pragma once

#include <stdint.h>
//#include <signal.h>
#include "lock.h"
#include "interrupts.h"
#include "global_defs.h"
#include "lib/hashmap.h"
#include "lib/vector.h"

extern void proc_yield();

// const int PROCESS_STACK_SIZE = 0x4000;

struct proc_vmm_map
{

    void *virt;
    void *phys;
    VECTOR_TYPE(void *)
    pages;
    uint64_t page_count;
};

struct process
{

    // pid_t id;
    uint64_t privilege_level;
    // char[128] proc_name;
    char **argv;
    char **envr;
    uint64_t *kernel_stack;
    uint64_t *ist_stack;
    uint64_t *stack;
    uint64_t rip;
    uint64_t rsp;
    uint64_t cr3;
    uint64_t rflags;
    uint64_t sleep_ticks;
    // struct sigaction sig_handlers[SIGNAL_MAX];
};
