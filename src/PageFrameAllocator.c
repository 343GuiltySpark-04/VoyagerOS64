#include "include/limine.h"
#include <stdint.h>
#include "include/paging/PageFrameAllocator.h"
#include <stdbool.h>
#include <stddef.h>
#include "include/KernelUtils.h"

uint64_t freeMemory = 0;
uint64_t reservedMemory = 0;
uint64_t usedMemory = 0;
bool Initialized = false;

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

void ReadMemoryMap()
{

    if (Initialized)
        return;

    Initialized = true;

    void *largestFreeMemSeg = NULL;
    size_t largestFreeMemSegSize = 0;

    for (uint64_t i = 0; i < memmap_req.response->entry_count; i++)
    {

        if (memmap_req.response->entries[i]->type == LIMINE_MEMMAP_USABLE)
        {

            if (memmap_req.response->entries[i]->length > largestFreeMemSeg)
            {

                largestFreeMemSeg = (void *)memmap_req.response->entries[i]->base;
                largestFreeMemSegSize = memmap_req.response->entries[i]->length;
            }
        }
    }

    uint64_t memorySize = GetMemorySize(); // I think this will work the same?
    freeMemory = memorySize;

    uint64_t bitmapSize = memorySize / 0x1000 / 8 + 1;

    // InintBitmap(bitmapSize, largestFreeMemSeg);

    for (uint64_t i = 0; i < memmap_req.response->entry_count; i++)
    {

        if (memmap_req.response->entries[i]->type == LIMINE_MEMMAP_USABLE)
        {

            for (uint64_t index = 0, startIndex = memmap_req.response->entries[i]->base / 0x1000; index < memmap_req.response->entries[i]->length / 0x1000; index++)
            {

                // PageBitmap.Set(index + startIndex, false);

                freeMemory += 0x1000;
                reservedMemory -= 0x1000;
            }
        }
    }

    // LockPages(PageBitmap.buffer, PageBitmap.size / 0x1000 + 1);
}