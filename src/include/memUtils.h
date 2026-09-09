#pragma once
#ifndef MEMUTILS_H
#define MEMUTILS_H

#define SIZE 0x500000
#define BSIZE 16

/* Legacy allocation policy flags are retained for source compatibility with
 * archived allocator code, but the active kernel allocator is kmalloc. */
#define MALLOC_FLAGS_VIRTUAL (1 << 0)
#define MALLOC_FLAGS_PHYSICAL (1 << 1)
#define MALLOC_FLAGS_CLEAR (1 << 2)
#define MALLOC_FLAGS_LOW1MEG (1 << 3)
#define MALLOC_FLAGS_LOW16MEG (1 << 4)
#define MALLOC_FLAGS_LOW4GIG (1 << 5)
#define MALLOC_FLAGS_ALIGNED (1 << 6)
#define MALLOC_HARDWARE32 \
    (MALLOC_FLAGS_LOW4GIG | MALLOC_FLAGS_PHYSICAL | MALLOC_FLAGS_CLEAR)
#define MALLOC_HARDWARE64 (MALLOC_FLAGS_PHYSICAL | MALLOC_FLAGS_CLEAR)

#endif
