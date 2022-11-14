#include <stdint.h>
#include <stddef.h>
#include "include/gdt.h"
#include "include/idt.h"
#include "include/KernelUtils.h"
#include "include/serial.h"
#include "include/limine.h"
#include "include/printf.h"
#include "include/heap.h"
#include "include/kernel.h"
#include "include/paging/frameallocator.h"
#include "include/paging/paging.h"
#include "include/registers.h"
#include "include/memUtils.h"
#include "include/vgafont.h"
#include "include/pic.h"
#include "include/drivers/keyboard/keyboard.h"
#include "include/cpuUtils.h"
#include "include/terminal/framebuffer.h"
#include "include/terminal/term.h"
#include "include/liballoc.h"
#include "include/time.h"
#include "include/shell.h"
#include "include/sched.h"
#include "include/paging/vmm.h"
#include "include/acpi/acpi.h"

#define White "\033[1;00m"
#define Red "\033[1;31m"
#define Green "\033[1;32m"
#define Yellow "\033[1;33m"
#define Blue "\033[1;34m"
#define Purple "\033[1;35m"
#define Cyan "\033[1;36m"
#define Black "\033[1;37m"

uint32_t term_fg = 0x0055ff55;
uint32_t term_bg = 0x00000000;

KHEAPBM kheap;

/// @attention Limine requests can be placed anywhere, but it is important that
/// the compiler does not optimise them away, so, usually, they should
/// be made volatile or equivalent.

volatile struct limine_kernel_address_request Kaddress_req = {

    .id = LIMINE_KERNEL_ADDRESS_REQUEST,
    .revision = 0

};

volatile struct limine_terminal_request early_term = {

    .id = LIMINE_TERMINAL_REQUEST,
    .revision = 0

};

/// @brief breakpoint() Provides a magic breakpoint for debugging in Bochs
extern void breakpoint();
extern void stop_interrupts();
extern void start_interrupts();
extern void halt();
extern void task_switch_int();

/// @var allows me to enable or dissable or alter behavoir according to wether the kernel
/// is fully loaded yet.
uint32_t bootspace = 2;

uint8_t kerror_mode = 0;

struct term_context *term_context;

static struct PageTable *test_table;

/// \fn  following will be our kernel's entry point.
void _start(void)
{

    if (early_term.response == NULL || early_term.response->terminal_count < 1)
    {

        bootspace = 1;

        printf_("%s\n", "Bootloader Terminal Offline Using Serial Only!");
    }

    printf_("%s", "Early Terminal Using Framebuffer At Physical Address: ");
    printf_("0x%llx\n", TranslateToPhysicalMemoryAddress(early_term.response->terminals[0]->framebuffer));
    printf_("%s", "And At Virtual Address: ");
    printf_("0x%llx\n", early_term.response->terminals[0]->framebuffer);

    print_date();

    cpuid_readout();

    breakpoint();

    stop_interrupts();

    LoadGDT_Stage1();

    printf_("%s\n", "Loaded GDT");

    breakpoint();

    idt_init();

    printf_("%s\n", "Loaded IDT");

    pic_enable();

    printf_("%s\n", "PICs Online");

    time_init();

    idt_reg_test();

    print_memmap();

    // @brief Kernel Addresses
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
        printf_("%s\n", "--------------------------------------");
    }

    breakpoint();

    // read_memory_map();

    /// @brief print usable memory to log
    print_memory();

    init_memory();

    printf_("%s", "CR3: ");
    printf_("0x%llx\n", readCR3());
    printf_("%s", "CR0: ");
    printf_("0x%llx\n", readCRO());
    printf_("%s", "CR4: ");
    printf_("0x%llx\n", readCR4());

    acpi_init();

    keyboard_init();

    printf_("%s\n", "Handing Control to Standalone Terminal...");

    bootspace = 3;

    for (uint64_t i = 0; i < 500; i++)
    {

        printf_("%s\n", "");
    }

    early_term.response->write(early_term.response->terminals[0], NULL, LIMINE_TERMINAL_FULL_REFRESH);

    bootspace = 1;

    term_context = fbterm_init(malloc, fbr_req.response->framebuffers[0]->address, fbr_req.response->framebuffers[0]->width, fbr_req.response->framebuffers[0]->height,

                               fbr_req.response->framebuffers[0]->pitch, NULL, NULL, NULL, &term_bg, &term_fg, &vgafont, 8, 16, 1,

                               1, 1, 1);

    bootspace = 0;

    // VMM_table_clone();

    print_memory();

    print_load_time();

    print_date();

    printf_("%s\n", "VoyagerOS64 v0.0.4");

    printf_("%s", ":> ");

    //  init_multitasking();

    // Just chill until needed
    while (1)
    {
    }
}