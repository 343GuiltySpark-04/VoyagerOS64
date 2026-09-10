#include "include/KernelUtils.h"
#include "include/limine.h"
#include "include/printf.h"
#include <stddef.h>
#include <stdint.h>

extern volatile struct limine_memmap_request memmap_req;

const struct kswitches k_mode = {.stack_trace_size     = 24,
                                 .stack_trace_on_fault = 1,
                                 .acpi_support         = 0,
                                 .sched_debug          = 0,
                                 .addr_debug           = 0,
                                 .hw_rng_support       = 1,
                                 .mem_readout_unit     = 1,
                                 .liballoc_debug       = 0,
                                 .fpu_allowed          = 0,
                                 .timestamp            = 1};

uint64_t bytes_to_mib(uint64_t bytes)
{
    return bytes >> 20;
}

uint64_t bytes_to_gib(uint64_t bytes)
{
    return bytes >> 30;
}

uint64_t get_memory_size(void)
{
    uint64_t memory_size = 0;

    if (!memmap_req.response)
        return 0;

    for (size_t i = 0; i < memmap_req.response->entry_count; i++)
        memory_size += memmap_req.response->entries[i]->length;

    return bytes_to_mib(memory_size);
}

uint64_t get_memory_size_gib(void)
{
    uint64_t memory_size = 0;

    if (!memmap_req.response)
        return 0;

    for (size_t i = 0; i < memmap_req.response->entry_count; i++)
        memory_size += memmap_req.response->entries[i]->length;

    return bytes_to_gib(memory_size);
}

void print_memmap(void)
{
    if (!memmap_req.response)
    {
        printf_("%s\n", "Memory map unavailable.");
        return;
    }

    size_t size             = memmap_req.response->entry_count;
    size_t num_usable       = 0;
    size_t num_bad          = 0;
    size_t num_reclaim_bl   = 0;
    size_t num_reclaim_acpi = 0;

    printf_("%s\n", "--------------------------------------");
    printf_("%s\n", "|             MEMORY MAP             |");
    printf_("%s\n", "--------------------------------------");
    printf_("%s\n", "Type Legend: ");
    printf_("%s\n", "0 Usable");
    printf_("%s\n", "1 Reserved");
    printf_("%s\n", "2 ACPI Reclaimable");
    printf_("%s\n", "3 ACPI NVS");
    printf_("%s\n", "4 Bad Memory");
    printf_("%s\n", "5 Bootloader Reclaimable");
    printf_("%s\n", "6 Kernel And Modules");
    printf_("%s\n", "7 Framebuffer");
    printf_("%s\n", "--------------------------------------");
    printf_("Number Of Entries: %llu\n", (unsigned long long) size);

    for (size_t i = 0; i < size; ++i)
    {
        struct limine_memmap_entry *entry = memmap_req.response->entries[i];

        printf_("%s", "Info For Entry Number: ");
        printf_("%llu\n", (unsigned long long) (i + 1));
        printf_("Entry Base: 0x%llx\n", (unsigned long long) entry->base);
        printf_("Entry Limit: 0x%llx\n", (unsigned long long) entry->length);
        printf_("Entry Type: %llu\n", (unsigned long long) entry->type);
        printf_("%s\n", "--------------------------------------");

        if (entry->type == LIMINE_MEMMAP_USABLE)
            num_usable++;
        else if (entry->type == LIMINE_MEMMAP_BAD_MEMORY)
            num_bad++;
        else if (entry->type == LIMINE_MEMMAP_BOOTLOADER_RECLAIMABLE)
            num_reclaim_bl++;
        else if (entry->type == LIMINE_MEMMAP_ACPI_RECLAIMABLE)
            num_reclaim_acpi++;
    }

    printf_("Number of Usable Entries: %llu\n",
            (unsigned long long) num_usable);
    printf_("Number of Bad Entries: %llu\n", (unsigned long long) num_bad);
    printf_("Number of Bootloader Reclaimable Entries: %llu\n",
            (unsigned long long) num_reclaim_bl);
    printf_("Number of ACPI Reclaimable Entries: %llu\n",
            (unsigned long long) num_reclaim_acpi);
    printf_("Memory Size: %llu MiB.\n", (unsigned long long) get_memory_size());
    printf_("Memory Size: %llu GiB.\n",
            (unsigned long long) get_memory_size_gib());
    printf_("%s\n", "--------------------------------------");
}
