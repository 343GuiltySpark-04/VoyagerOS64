#include "include/limine.h"
#include <stdint.h>
#include "include/paging/frameallocator.h"
#include <stdbool.h>
#include <stddef.h>
#include "include/KernelUtils.h"

uint64_t freeMemory = 0;
uint64_t reservedMemory = 0;
uint64_t usedMemory = 0;
bool initialized = false;
uint8_t *bitmap = NULL;
uint64_t bitmapSize = 0;

const char *MemoryMapTypeString(int type)
{
    switch (type)
    {
    case LIMINE_MEMMAP_USABLE:

        return "Usable";

    case LIMINE_MEMMAP_RESERVED:

        return "Reserved";

    case LIMINE_MEMMAP_ACPI_RECLAIMABLE:

        return "ACPI Reclaimable";

    case LIMINE_MEMMAP_ACPI_NVS:

        return "ACPI NVS";

    case LIMINE_MEMMAP_BAD_MEMORY:

        return "Bad Memory";

    case LIMINE_MEMMAP_BOOTLOADER_RECLAIMABLE:

        return "Bootloader Reclaimable";

    case LIMINE_MEMMAP_KERNEL_AND_MODULES:

        return "Kernel and Modules";

    case LIMINE_MEMMAP_FRAMEBUFFER:

        return "Framebuffer";

    default:

        return "Unknown";
    }
}

void read_memory_map()
{
    if (initialized)
        return;

    initialized = true;

    void *largestFreeMemSegment = NULL;
    size_t largestFreeMemSegmentSize = 0;

    for (uint64_t i = 0; i < memmap_req.response->entry_count; i++)
    {
        if (memmap_req.response->entries[i]->type == LIMINE_MEMMAP_USABLE && memmap_req.response->entries[i]->length > largestFreeMemSegment)
        {
            largestFreeMemSegment = (void *)memmap_req.response->entries[i]->base;
            largestFreeMemSegmentSize = memmap_req.response->entries[i]->length;
        }
    }

    uint64_t memorySize = get_memory_size(); // I think this will work the same?
    freeMemory = memorySize;

    bitmapSize = memorySize / 0x1000 / 8 + 1;

    bitmap = (uint8_t *)largestFreeMemSegment;

    for(uint64_t i = 0; i < bitmapSize * 8; i++)
    {
        bitmap_set(bitmap, i, true);

        freeMemory -= 0x1000;
        reservedMemory += 0x1000;
    }

    for (uint64_t i = 0; i < memmap_req.response->entry_count; i++)
    {
        if (memmap_req.response->entries[i]->type == LIMINE_MEMMAP_USABLE)
        {
            for (uint64_t index = 0, startIndex = memmap_req.response->entries[i]->base / 0x1000;
                index < memmap_req.response->entries[i]->length / 0x1000; index++)
            {
                bitmap_set(bitmap, index + startIndex, false);

                freeMemory += 0x1000;
                reservedMemory -= 0x1000;
            }
        }
    }

    frame_lock(bitmapSize / 0x1000 + 1);
}

void *frame_request()
{
    for (uint64_t i = 0; i < bitmapSize * 8; i++)
    {
        if (bitmap_get(bitmap, i) == true) continue;

        frame_lock((void*)(i * 0x1000));

        return (void*)(i * 0x1000);
    }

    return NULL; // Page Frame Swap to file
}

void *frame_request_multiple(uint32_t count)
{
    uint32_t freeCount = 0;

    for (uint32_t i = 0; i < bitmapSize * 8; i++)
    {
        if (bitmap_get(bitmap, i) == false)
        {
            if(freeCount == count)
            {
                uint32_t index = i - count;

                frame_lock_multiple((void*)((uint64_t)index * 0x1000), count);

                return (void *)((uint64_t)index * 0x1000);
            }

            freeCount++;
        }
        else
        {
            freeCount = 0;
        }
    }

    return NULL;
}

void frame_free(void *address)
{
    uint64_t index = (uint64_t)address / 0x1000;

    if(bitmap_get(bitmap, index) == false)
    {
        return;
    }

    bitmap_set(bitmap, index, false);

    freeMemory += 0x1000;
    usedMemory -= 0x1000;
}

void frame_free_multiple(void *address, uint64_t pageCount)
{
    for(uint64_t t = 0; t < pageCount; t++)
    {
        frame_free((void *)((uint64_t)address + (t * 0x1000)));
    }
}

void frame_lock(void *address)
{
    uint64_t index = (uint64_t)address / 0x1000;

    if (bitmap_get(bitmap, index) == true)
    {
        return;
    }

    bitmap_set(bitmap, index, true);

    freeMemory -= 0x1000;
    usedMemory += 0x1000;
}

void frame_lock_multiple(void *address, uint64_t pageCount)
{
    for(uint64_t t = 0; t < pageCount; t++)
    {
        frame_lock((void *)((uint64_t)address + (t * 0x1000)));
    }
}

uint64_t free_ram()
{
    return freeMemory;
}

uint64_t used_ram()
{
    return usedMemory;
}

uint64_t reserved_ram()
{
    return reservedMemory;
}