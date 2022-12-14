#include "include/KernelUtils.h"
#include <stddef.h>
#include <stdint.h>
#include "include/limine.h"
#include "include/paging/frameallocator.h"
#include "include/paging/paging.h"
#include "include/string.h"
#include "include/registers.h"

extern void breakpoint();
extern uint8_t *frameBitmap;
extern void halt();

typedef char symbol[];

extern symbol text_start_addr, text_end_addr,
    rodata_start_addr, rodata_end_addr,
    data_start_addr, data_end_addr;

volatile struct limine_memmap_request memmap_req = {
    .id = LIMINE_MEMMAP_REQUEST,
    .revision = 0};

volatile struct limine_framebuffer_request fbr_req = {

    .id = LIMINE_FRAMEBUFFER_REQUEST,
    .revision = 0

};

extern volatile struct limine_kernel_address_request Kaddress_req;

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

    int num_useable = 0;
    int num_bad = 0;
    int num_reclaim_bl = 0;
    int num_reclaim_acpi = 0;

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

        if (memmap_req.response->entries[i]->type == 0)
        {

            num_useable++;
        }

        if (memmap_req.response->entries[i]->type == 4)
        {

            num_bad++;
        }

        if (memmap_req.response->entries[i]->type == 5)
        {

            num_reclaim_bl++;
        }

        if (memmap_req.response->entries[i]->type == 2)
        {

            num_reclaim_acpi++;
        }
    }

    printf_("%s", "Number of Usable Entries: ");
    printf_("%i\n", num_useable);
    printf_("%s", "Number of Bad Entries: ");
    printf_("%i\n", num_bad);
    printf_("%s", "Number of Bootloader Reclaimable Entries: ");
    printf_("%i\n", num_reclaim_bl);
    printf_("%s", "Number of ACPI Reclaimable Entries: ");
    printf_("%i\n", num_reclaim_acpi);
    printf_("%s", "Memory Size: ");
    printf_("0x%llx\n", get_memory_size());
    printf_("%s\n", "--------------------------------------");
}

static struct PageTable *page_table;

void init_memory()
{
    read_memory_map();

    page_table = (struct PageTable *)frame_request();

    memset(page_table, 0, sizeof(struct PageTable));

    printf_("%s\n", "Initializing Paging");

    breakpoint();

    printf_("%s\n", "Preallocating Upper Region");

    for (uint64_t i = 256; i < 512; i++)
    {

        void *page = frame_request();

        if (page == 0x0)
        {

            printf_("%s\n", "!!!Kernel Panic!!!");
            printf_("%s", "Attempt to preallocate page with invalid address at table index: ");
            printf_("%i\n", i);
            printf_("%s", "Requested address: ");
            printf_("0x%llx\n", page);
            printf_("%s\n", "!!!Kernel Panic!!!");
            halt();
        }

        memset(page, 0, 0x1000);

        page_table->entries[i] = (uint64_t)page | PAGING_FLAG_PRESENT | PAGING_FLAG_WRITABLE;
    }

    // Enable Write Protection
    writeCR0(readCRO() | (1 << 16));

    // Program the PAT
    writeMSR(0x0277, 0x0000000005010406);

    printf_("%s\n", "Mapping Memory Map");

#define ALIGN_DOWN(value, align) ((value / align) * align)
#define ALIGN_UP(value, align) (((value + (align - 1)) / align) * align)

    uint64_t textStart = ALIGN_DOWN((uint64_t)text_start_addr, 0x1000);
    uint64_t textEnd = ALIGN_UP((uint64_t)text_end_addr, 0x1000);
    uint64_t rodataStart = ALIGN_DOWN((uint64_t)rodata_start_addr, 0x1000);
    uint64_t rodataEnd = ALIGN_UP((uint64_t)rodata_end_addr, 0x1000);
    uint64_t dataStart = ALIGN_DOWN((uint64_t)data_start_addr, 0x1000);
    uint64_t dataEnd = ALIGN_UP((uint64_t)data_end_addr, 0x1000);

    printf_("map text\n");

    for (uint64_t textAddress = textStart; textAddress < textEnd; textAddress += 0x1000)
    {
        uint64_t target = textAddress - Kaddress_req.response->virtual_base + Kaddress_req.response->physical_base;

        PagingMapMemory(page_table, (void *)textAddress, (void *)target, PAGING_FLAG_PRESENT);
    }

    printf_("map rodata\n");

    for (uint64_t rodataAddress = rodataStart; rodataAddress < rodataEnd; rodataAddress += 0x1000)
    {
        uint64_t target = rodataAddress - Kaddress_req.response->virtual_base + Kaddress_req.response->physical_base;

        PagingMapMemory(page_table, (void *)rodataAddress, (void *)target, PAGING_FLAG_PRESENT | PAGING_FLAG_NO_EXECUTE);
    }

    printf_("map data\n");

    for (uint64_t dataAddress = dataStart; dataAddress < dataEnd; dataAddress += 0x1000)
    {
        uint64_t target = dataAddress - Kaddress_req.response->virtual_base + Kaddress_req.response->physical_base;

        PagingMapMemory(page_table, (void *)dataAddress, (void *)target, PAGING_FLAG_PRESENT | PAGING_FLAG_WRITABLE | PAGING_FLAG_NO_EXECUTE);
    }

    printf_("map global memory\n");

    for (uintptr_t addr = 0x1000; addr < 0x100000000; addr += 0x1000)
    {
        PagingIdentityMap(page_table, addr, PAGING_FLAG_PRESENT | PAGING_FLAG_WRITABLE);
        PagingMapMemory(page_table, TranslateToHighHalfMemoryAddress(addr), addr, PAGING_FLAG_PRESENT | PAGING_FLAG_WRITABLE | PAGING_FLAG_NO_EXECUTE);
    }

    printf_("map kernel and modules\n");

    for (uint64_t i = 0; i < memmap_req.response->entry_count; i++)
    {
        struct limine_memmap_entry *entry = memmap_req.response->entries[i];

        uint64_t base = ALIGN_DOWN(entry->base, 0x1000);
        uint64_t top = ALIGN_UP(entry->base + entry->length, 0x1000);

        if (top <= 0x100000000)
        {
            continue;
        }

        for (uint64_t j = base; j < top; j += 0x1000)
        {

            if (j < 0x100000000)
            {
                continue;
            }

            PagingIdentityMap(page_table, j, PAGING_FLAG_PRESENT | PAGING_FLAG_WRITABLE);
            PagingMapMemory(page_table, TranslateToHighHalfMemoryAddress(j), j, PAGING_FLAG_PRESENT | PAGING_FLAG_WRITABLE | PAGING_FLAG_NO_EXECUTE);
        }
    }

    frameBitmap = (uint8_t *)TranslateToHighHalfMemoryAddress(frameBitmap);

    writeCR3((uint64_t)page_table);

    printf_("Wrote CR3\n");

    breakpoint();
}

void print_memory()
{

    printf_("Total Memory: %llu", get_memory_size() / 1000);
    printf_("%s\n", "Kb.");
    printf_("Free Memory: %llu", free_ram() / 1000);
    printf_("%s\n", "Kb.");
    printf_("Used Memory: %llu", used_ram() / 1000);
    printf_("%s\n", "Kb.");
    printf_("Reserved Memory: %llu", reserved_ram() / 1000);
    printf_("%s\n", "Kb.");
}
