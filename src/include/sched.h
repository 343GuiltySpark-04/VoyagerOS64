#pragma once

#ifndef _SCHED_H
#define _SCHED_H

#include <stdint.h>
#include <stddef.h>
#include "global_defs.h"
#define TASK_RUNNING 1

typedef struct
{

    struct
    {

        uint64_t rdi;
        uint64_t rsi;
        uint64_t rdx;
        uint64_t rcx;
        uint64_t rbx;
        uint64_t rax;
    } registers;

    struct {

        uint64_t top;
        uint64_t base;

    } stack;

    uint64_t head;
    uint64_t page_table;
} task_ctx_t;

#endif