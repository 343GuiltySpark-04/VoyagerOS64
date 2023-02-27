#pragma once
#include <stdint.h>
#include "limine.h"
#include "global_defs.h"

#ifndef _KERNEL_UTILS_H
#define _KERNEL_UTILS_H

extern volatile struct limine_memmap_request memmap_req;

extern volatile struct limine_framebuffer_request fbr_req;

extern const struct kswitches k_mode;

extern struct PageTable *page_table;

// struct contains kernel behavior setting switches. their values are set in KernelUtils.c
struct PACKED kswitches
{

    // how many frames back to trace (used as default a differant value can be set in the args)
    uint8_t stack_trace_size;

    // a value of 1 or 0 or 2, 1 enables 0 disables 2 sets to dump.
    uint8_t stack_trace_on_fault;

    // a value of 1 or 0, 1 enables 0 disables.
    uint8_t acpi_support;

    // a value of 1 or 0, 1 enables 0 disables.
    uint8_t sched_debug;

    // a value of 1 or 0, 1 enables 0 disables. I suggest running with 256mb of ram to avoid a log from hell.
    uint8_t addr_debug;

    uint8_t hw_rng_support;

    // 1 mb, 2 kb, 3 b
    uint8_t mem_readout_unit;
};

void print_memmap();

uint64_t get_memory_size();

void init_memory();

void print_memory();

extern int temp;

extern void restore_floats();

extern void save_floats();

extern struct term_context *term_context;

#endif