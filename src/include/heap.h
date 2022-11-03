#pragma once
#include <stdint.h>

typedef struct _KHEAPBLOCKBM
{
    struct _KHEAPBLOCKBM *next;
    uint64_t size;
    uint64_t used;
    uint64_t bsize;
    uint64_t lfb;
    uintptr_t data;
    uint8_t *bm;
} KHEAPBLOCKBM;

typedef struct _KHEAPBM
{
    KHEAPBLOCKBM *fblock;
} KHEAPBM;

void k_heapBMInit(KHEAPBM *heap);
int k_heapBMAddBlock(KHEAPBM *heap, uintptr_t addr, uint64_t size, uint64_t bsize);
int k_heapBMAddBlockEx(KHEAPBM *heap, uintptr_t addr, uint64_t size, uint64_t bsize, KHEAPBLOCKBM *b, uint8_t *bm, uint8_t isBMInside);
void *k_heapBMAlloc(KHEAPBM *heap, uint64_t size);
void k_heapBMFree(KHEAPBM *heap, void *ptr);
uintptr_t k_heapBMGetBMSize(uintptr_t size, uint64_t bsize);
void *k_heapBMAllocBound(KHEAPBM *heap, uint64_t size, uint64_t mask);
void k_heapBMSet(KHEAPBM *heap, uintptr_t ptr, uintptr_t size, uint8_t rval);
