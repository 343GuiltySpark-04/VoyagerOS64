#include "include/limine.h"
#include <stdint.h>
#include "include/paging/frameallocator.h"
#include <stdbool.h>
#include <stddef.h>
#include "include/KernelUtils.h"
#include "include/printf.h"

uint64_t freeMemory = 0;
uint64_t reservedMemory = 0;
uint64_t usedMemory = 0;
bool initialized = false;
uint8_t *frameBitmap = NULL;
uint64_t bitmapSize = 0;

extern void halt();

/**
* @brief Convert a memory map type to a string.
* @param type The memory map type to convert.
* @return Pointer to a string describing the memory map type
*/
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

/**
* @brief Read and process memory map
* @return true if successful else false
*/
void read_memory_map()
{
    if (initialized)
        return;

    initialized = true;

    void *largestFreeMemSegment = NULL;
    size_t largestFreeMemSegmentSize = 0;
    uint64_t bitmap_entry;

    uint64_t memorySize = get_memory_size(); // I think this will work the same?
    freeMemory = memorySize;

    bitmapSize = memorySize / 0x1000 / 8 + 1;

    printf_("%s", "Bitmap Size is: ");
    printf_("0x%llx\n", bitmapSize);

    for (uint64_t i = 0; i < memmap_req.response->entry_count; i++)
    {
        struct limine_memmap_entry *entry = memmap_req.response->entries[i];

        if (entry->type == LIMINE_MEMMAP_USABLE && entry->length > bitmapSize)
        {
            largestFreeMemSegment = (void *)entry->base;
            largestFreeMemSegmentSize = entry->length;
            bitmap_entry = i;
        }
    }

    if (largestFreeMemSegment == NULL)
    {
        printf_("%s\n", "!!!Kernel Panic!!!");
        printf_("%s\n", "No Suitable Memory Map Entries Found!");
        printf_("%s\n", "!!!Kernel Panic!!!");
        halt();
    }

    printf_("%s", "Address of Largest Free Entry: ");
    printf_("0x%llx\n", (uint64_t)largestFreeMemSegment);

    printf_("%s", "Size of Largest Free Entry: ");
    printf_("0x%llx\n", largestFreeMemSegmentSize);

    memmap_req.response->entries[bitmap_entry]->length -= bitmapSize;

    largestFreeMemSegmentSize = memmap_req.response->entries[bitmap_entry]->length;

    printf_("%s", "Size of Largest Free Entry Post Bitmap Allocation: ");
    printf_("0x%llx\n", largestFreeMemSegmentSize);

    // print_memmap();

    frameBitmap = (uint8_t *)largestFreeMemSegment;

    for (uint64_t i = 0; i < bitmapSize * 8; i++)
    {
        bitmap_set(frameBitmap, i, true);

        freeMemory -= 0x1000;
        reservedMemory += 0x1000;
    }

    for (uint64_t i = 0; i < memmap_req.response->entry_count; i++)
    {
        struct limine_memmap_entry *entry = memmap_req.response->entries[i];

        if (entry->type == LIMINE_MEMMAP_USABLE)
        {
            printf("Unlocking usable pages for %p-%p (%llu pages)\n", entry->base, entry->base + entry->length, entry->length / 0x1000);

            for (uint64_t index = 0, startIndex = entry->base / 0x1000;
                 index < entry->length / 0x1000; index++)
            {
                bitmap_set(frameBitmap, index + startIndex, false);

                freeMemory += 0x1000;
                reservedMemory -= 0x1000;
            }
        }
    }

    frame_lock_multiple(frameBitmap, bitmapSize / 0x1000 + 1);
}

/**
* @brief Request a frame from page.
* @return Pointer to frame or NULL if none
*/
void *frame_request()
{
    for (uint64_t i = 0; i < bitmapSize * 8; i++)
    {
        if (bitmap_get(frameBitmap, i) == true)
            continue;

        frame_lock((void *)(i * 0x1000));

        return (void *)(i * 0x1000);
    }

    return NULL; // Page Frame Swap to file
}

/**
* @brief Print the frame bitmap to stdout.
* @return void. Side effects : None
*/
void print_frame_bitmap()
{

    printf_("%s\n", "Frame Bitmap Readout Start");
    printf_("%s\n", "------------------------------------------------");

    for (uint64_t i = 0; i < bitmapSize * 8; i++)
    {

        printf_("%s", "Data For Frame Bitmap Entry: ");
        printf_("0x%llx\n", i);
        printf_("%s", "Data: ");
        printf_("0x%llx\n", bitmap_get(frameBitmap, i));
    }

    printf_("%s\n", "------------------------------------------------");
}

/**
* @brief Request multiple frames from the frame bitmap
* @param count Number of frames to request
* @return Pointer to frame or NULL if there isn't
*/
void *frame_request_multiple(uint32_t count)
{
    uint32_t freeCount = 0;

    for (uint32_t i = 0; i < bitmapSize * 8; i++)
    {
        if (bitmap_get(frameBitmap, i) == false)
        {
            if (freeCount == count)
            {
                uint32_t index = i - count;

                frame_lock_multiple((void *)((uint64_t)index * 0x1000), count);

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

/**
* @brief Free a frame. This is called by memory_deallocate () to indicate that it is no longer needed
* @param * address
* @return void * - true if
*/
void frame_free(void *address)
{
    uint64_t index = (uint64_t)address / 0x1000;

    if (bitmap_get(frameBitmap, index) == false)
    {
        return;
    }

    bitmap_set(frameBitmap, index, false);

    freeMemory += 0x1000;
    usedMemory -= 0x1000;
}

/**
* @brief Free multiple pages of memory
* @param * address
* @param pageCount Number of pages to
*/
void frame_free_multiple(void *address, uint64_t pageCount)
{
    for (uint64_t t = 0; t < pageCount; t++)
    {
        frame_free((void *)((uint64_t)address + (t * 0x1000)));
    }
}

/**
* @brief Lock the frame at the given address.
* @param * address
* @return True if the lock was acquired
*/
void frame_lock(void *address)
{
    uint64_t index = (uint64_t)address / 0x1000;

    if (bitmap_get(frameBitmap, index) == true)
    {
        return;
    }

    bitmap_set(frameBitmap, index, true);

    freeMemory -= 0x1000;
    usedMemory += 0x1000;
}

/**
* @brief Lock a range of pages
* @param * address
* @param pageCount The number of pages to
*/
void frame_lock_multiple(void *address, uint64_t pageCount)
{
    for (uint64_t t = 0; t < pageCount; t++)
    {
        frame_lock((void *)((uint64_t)address + (t * 0x1000)));
    }
}

/**
* @brief Returns the amount of free RAM in kilobytes.
* @return Amount of free RAM in kiloby
*/
uint64_t free_ram()
{
    return freeMemory;
}

/**
* @brief Returns the amount of RAM used by the host.
* @return The amount of RAM used by the host
*/
uint64_t used_ram()
{
    return usedMemory;
}

/**
* @brief Get the amount of RAM reserved for this instance.
* @return The amount of RAM reserved
*/
uint64_t reserved_ram()
{
    return reservedMemory;
}