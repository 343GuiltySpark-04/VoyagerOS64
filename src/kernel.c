#include <stdint.h>
#include <stddef.h>
#include "include/gdt.h"
#include "include/idt.h"
#include "include/KernelUtils.h"
#include "include/serial.h"
#include "include/limine.h"
#include "include/printf.h"
#include "include/memUtils.h"

#define White "\033[1;00m"
#define Red "\033[1;31m"
#define Green "\033[1;32m"
#define Yellow "\033[1;33m"
#define Blue "\033[1;34m"
#define Purple "\033[1;35m"
#define Cyan "\033[1;36m"
#define Black "\033[1;37m"


// The Limine requests can be placed anywhere, but it is important that
// the compiler does not optimise them away, so, usually, they should
// be made volatile or equivalent.

static volatile struct limine_memmap_request memmap_req = {
    .id = LIMINE_MEMMAP_REQUEST,
    .revision = 0

};

static volatile struct limine_kernel_address_request Kaddress_req = {

    .id = LIMINE_KERNEL_ADDRESS_REQUEST,
    .revision = 0

};

extern void breakpoint();
extern void stop_interrupts();
extern void start_interrupts();

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
}

// The following will be our kernel's entry point.
void _start(void)
{
    breakpoint();

    stop_interrupts();

    LoadGDT_Stage1();

    printf_("%s\n", "Loaded GDT");

    breakpoint();

    idt_init();

    printf_("%s\n", "Loaded IDT");

    if (memmap_req.response == NULL || memmap_req.response->entry_count < 1)
    {

        printf_("%s\n", "!!!Error While Fetching Memory Map!!!");
    }
    else
    {

        printf_("%s\n", "Memory Map Retrieved");
        printf_("%s", "Number of entries retrived: ");
        printf_("%i\n", memmap_req.response->entry_count);
    }

    print_memmap();

    // Retrive Kernel Addresses
    if (Kaddress_req.response == NULL)
    {

        printf_("%s\n", "!!!Error While Fetching Kernel Addresses!!!");
    }
    else
    {

        printf_("%s\n", "Kernel Base Addresses Are As Follows: ");
        printf_("%s", "Physical Address: ");
        printf_("0x%llx\n", Kaddress_req.response->physical_base);
        printf_("%s", "Virtual Address: ");
        printf_("0x%llx\n", Kaddress_req.response->virtual_base);
        printf_("%s", "Address From Memory Map: ");
        printf_("0x%llx\n", memmap_req.response->entries[6]->base);
        printf_("%s", "Limit From Memory Map: ");
        printf_("0x%llx\n", memmap_req.response->entries[6]->length);
        printf_("%s\n", "--------------------------------------");
    }

 

    printf_("%s\n", "Kernel Loaded");

    print_memmap();

    // Just chill until needed
    while (1)
    {
    }
}