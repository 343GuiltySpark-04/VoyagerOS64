#include "include/KernelUtils.h"
#include <stddef.h>
#include <stdint.h>
#include "include/limine.h"
#include "include/paging/frameallocator.h"
#include "include/paging/paging.h"
#include "include/string.h"
#include "include/registers.h"

volatile struct limine_memmap_request memmap_req = {
    .id = LIMINE_MEMMAP_REQUEST,
    .revision = 0};

uint64_t get_memory_size()
{
    static uint64_t memorySize = 0;

    if (memorySize > 0)
    {
        return memorySize;
    }

    for (uint64_t i = 0; i < memmap_req.response->entry_count; i++)
    {
        memorySize += memmap_req.response->entries[i]->length;
    }

    return memorySize;
}

void print_memmap()
{
    int size = memmap_req.response->entry_count;

    printf_("%s\n", "--------------------------------------");
    printf_("%s\n", "|             MEMORY MAP             |");
    printf_("%s\n", "--------------------------------------");
    printf_("%s\n", "Type Legend: ");

    printf_("%s", "Usable: ");
    printf_("%i\n", 0);
    printf_("%s", "Reserved: ");
    printf_("%i\n", 1);
    printf_("%s", "ACPI Reclaimable: ");
    printf_("%i\n", 2);
    printf_("%s", "ACPI NVS: ");
    printf_("%i\n", 3);
    printf_("%s", "Bad Memory: ");
    printf_("%i\n", 4);
    printf_("%s", "Bootloader Reclaimable: ");
    printf_("%i\n", 5);
    printf_("%s", "Kernel And Modules: ");
    printf_("%i\n", 6);
    printf_("%s", "Framebuffer: ");
    printf_("%i\n", 7);
    printf_("%s\n", "--------------------------------------");
    printf_("%s", "Number Of Entries: ");
    printf_("%i\n", size);
    printf_("%s\n", "--------------------------------------");

    printf_("%s\n", "Memory Map Is As Follows: ");

    for (size_t i = 0; i < size; ++i)
    {

        printf_("%s", "Info For Entry Number: ");
        printf_("%i\n", i + 1);
        printf_("%s", "Entry Base: ");
        printf_("0x%llx\n", memmap_req.response->entries[i]->base);
        printf_("%s", "Entry Limit: ");
        printf_("0x%llx\n", memmap_req.response->entries[i]->length);
        printf_("%s", "Entry Type: ");
        printf_("%i\n", memmap_req.response->entries[i]->type);
        printf_("%s\n", "--------------------------------------");
    }

    printf_("%s", "Memory Size: ");
    printf_("0x%llx\n", get_memory_size());
    printf_("%s\n", "--------------------------------------");
}

struct PageTable page_table;

void init_memory()
{

    read_memory_map();

    printf_("%s\n", "Initializing Paging");

    for (uint64_t i = 256; i < 512; i++)
    {

        void *page = frame_request();
        memset(page, 0, 0x1000);

        page_table.entries[i] = (uint64_t)page | PAGING_FLAG_PRESENT | PAGING_FLAG_WRITABLE | PAGING_FLAG_USER_ACCESSIBLE;
    }

    // Enable Write Protection
    writeCR0(readCRO() | (1 << 16));

    writeMSR(0x0277, 0x0000000005010406);

    printf_("%s\n", "Mapping Whole Memory");

    for (uint64_t index = 0; index < get_memory_size(); index += 0x1000)
    {

        PagingMapMemory(page_table.entries[index], TranslateToHighHalfMemoryAddress(index), (void *)(index),
                        PAGING_FLAG_PRESENT | PAGING_FLAG_WRITABLE);
    }

    printf_("%s\n", "Mapping Memory Map");

    for (uint64_t i = 0; i < memmap_req.response->entry_count; i++)
    {

        if (memmap_req.response->entries[i]->type == LIMINE_MEMMAP_KERNEL_AND_MODULES)
        {

            for (uint64_t index = 0; index < memmap_req.response->entries[i]->length / 0x1000 + 1; index++)
            {

                auto base = memmap_req.response->entries[i]->base;

                if (TranslateToKernelMemoryAddress(memmap_req.response->entries[i]->base) <= 0xFFFFFFFF90000000)
                {

                    base = TranslateToKernelMemoryAddress(base);
                }
                else
                {

                    base = TranslateToHighHalfMemoryAddress(base);
                }

                PagingMapMemory(page_table.entries[index], (void *)(base + index * 0x1000), (void *)(memmap_req.response->entries[i]->base + index * 0x1000),
                                PAGING_FLAG_PRESENT | PAGING_FLAG_WRITABLE);

                // have Fox look at this bit.
                /* pageTableManager.MapMemory((void *)(base + index * 0x1000), (void *)(desc->base + index * 0x1000),
                PAGING_FLAG_PRESENT | PAGING_FLAG_WRITABLE); */
            }
        }
        else if (memmap_req.response->entries[i]->type != LIMINE_MEMMAP_USABLE)
        {

            for (uint64_t index = 0; index < memmap_req.response->entries[i]->length / 0x1000 + 1; index++)
            {

                PagingMapMemory(page_table.entries[index], (void *)TranslateToHighHalfMemoryAddress(memmap_req.response->entries[i]->base + index * 0x1000),
                                (void *)(memmap_req.response->entries[i]->base + index * 0x1000), PAGING_FLAG_PRESENT | PAGING_FLAG_WRITABLE);
            }
        }
    }
}