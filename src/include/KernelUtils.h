#pragma once
#include <stdint.h>
#include "limine.h"

#ifndef _KERNEL_UTILS_H
#define _KERNEL_UTILS_H

extern volatile struct limine_memmap_request memmap_req;

extern volatile struct limine_framebuffer_request fbr_req;

void print_memmap();

uint64_t get_memory_size();

void init_memory();

void print_memory();

static struct PageTable *page_table;

extern struct term_context *term_context;

#endif