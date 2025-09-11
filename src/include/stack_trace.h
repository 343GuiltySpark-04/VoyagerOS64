#pragma once

#ifndef _STACK_TRACE_H
#define _STACK_TRACE_H
#include <stdint.h>
#include "global_defs.h"
#include <stdbool.h>
#include "limine.h"

// struct stack_return stack_frame_return;

extern volatile struct limine_stack_size_request stack_req;

struct stack_return
{

    uint64_t *addresses;

    uint64_t array_size;
};

struct stack_frame
{

    struct stack_frame *rbp;
    uint64_t rip;
};

void stack_trace_asm(uint64_t max_size, bool stop);
void print_stack_size();
void stack_trace(uint64_t max_frames);
void dump_hex(const void *data, size_t size);
// set leave to 0 to simply return otherwise set to 1
extern void stack_dump_asm(uint64_t leave, uint64_t trace_size);
void stack_dump();
void stack_dump_recursive(uint64_t max_frames);
#endif