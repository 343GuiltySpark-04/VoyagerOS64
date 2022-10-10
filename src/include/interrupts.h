#pragma once
#include <stdint.h>
#include "global_defs.h"

struct PACKED cpu_state
{

    uint64_t rax;
    uint64_t rbx;
    uint64_t rcx;
    uint64_t rdx;
    uint64_t rsi;
    uint64_t rdi;
    uint64_t rbp;
    uint64_t r8;
    uint64_t r9;
    uint64_t r10;
    uint64_t r11;
    uint64_t r12;
    uint64_t r13;
    uint64_t r14;
    uint64_t r15;
};

struct PACKED stack_state
{

    uint64_t error_code;
    uint64_t rip;
    uint64_t cs;
    uint64_t rflags;
};

void interrupt_handler();