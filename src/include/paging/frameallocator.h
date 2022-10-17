#pragma once
#include "../limine.h"
#include <stdint.h>
#include "../bitmap.h"

#ifdef __cplusplus
#define FRAME_EXPORT extern "C"
#else
#define FRAME_EXPORT
#endif

void read_memory_map();

void frame_free(void *address);
FRAME_EXPORT void frame_free_multiple(void *address, uint64_t pageCount);
void frame_lock(void *address);
void frame_lock_multiple(void *address, uint64_t pageCount);
void *frame_request();
FRAME_EXPORT void *frame_request_multiple(uint32_t count);
uint64_t free_ram();
uint64_t used_ram();
uint64_t reserved_ram();
