#pragma once
#include <stdint.h>
#include "limine.h"

extern volatile struct limine_memmap_request memmap_req;

void print_memmap();

uint64_t get_memory_size();

void init_memory();