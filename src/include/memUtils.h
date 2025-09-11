#pragma once
#ifndef MEMUTILS_H
#define MEMUTILS_H

#include "heap.h"
#include "liballoc.h"

extern KHEAPBM kheap;

#define SIZE 0x500000
#define BSIZE 16

#define ALLOC(TYPE) (malloc(sizeof(TYPE)))

#define MALLOC_FLAGS_VIRTUAL (1 << 0)  // any memory, physical or linear, non contiguous, any location
#define MALLOC_FLAGS_PHYSICAL (1 << 1) // must be physical address and contiguous, inclusive
#define MALLOC_FLAGS_CLEAR (1 << 2)    // clear the memory on return
#define MALLOC_FLAGS_LOW1MEG (1 << 3)  // must be before the 1 Meg mark, inclusive
#define MALLOC_FLAGS_LOW16MEG (1 << 4) // must be before the 16 Meg mark, inclusive
#define MALLOC_FLAGS_LOW4GIG (1 << 5)  // must be before the 4 Gig mark, inclusive
#define MALLOC_FLAGS_ALIGNED (1 << 6)  // must be aligned.  The aligned parameter is now used, else it is ignored.
#define MALLOC_HARDWARE32 (MALLOC_FLAGS_LOW4GIG | MALLOC_FLAGS_PHYSICAL | MALLOC_FLAGS_CLEAR)
#define MALLOC_HARDWARE64 (MALLOC_FLAGS_PHYSICAL | MALLOC_FLAGS_CLEAR)

#endif