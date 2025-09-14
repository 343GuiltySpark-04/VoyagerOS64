/**
 * Copyright (c) 2025 Tristan Adams
 *
 * This software is released under the MIT License.
 * https://opensource.org/licenses/MIT
 */

#include "include/kernel.h"
#include "include/KernelUtils.h"
#include "include/acpi/acpi.h"
#include "include/apic/lapic.h"
#include "include/cpuUtils.h"
#include "include/drivers/keyboard/keyboard.h"
#include "include/early_alloc.h"
#include "include/gdt.h"
#include "include/heap.h"
#include "include/idt.h"
#include "include/io.h"
#include "include/liballoc.h"
#include "include/limine.h"
#include "include/memUtils.h"
#include "include/mm/kheap.h"
#include "include/mm/kmalloc.h"
#include "include/paging/frame_supplier.h"
#include "include/paging/frameallocator.h"
#include "include/paging/neo_framealloc.h"
#include "include/paging/paging.h"
#include "include/paging/paging_bootstrap.h"
#include "include/paging/vmm.h"
#include "include/pebble.h"
#include "include/pic.h"
#include "include/printf.h"
#include "include/registers.h"
#include "include/sched.h"
#include "include/serial.h"
#include "include/shell.h"
#include "include/stack_trace.h"
#include "include/terminal/framebuffer.h"
#include "include/terminal/term.h"
#include "include/time.h"
#include "include/vgafont.h"
#include <stddef.h>
#include <stdint.h>

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

extern volatile struct limine_kernel_address_request Kaddress_req;

extern volatile struct limine_terminal_request early_term;

extern void     breakpoint();
extern void     stop_interrupts();
extern void     start_interrupts();
extern void     halt();
extern void     task_switch_int();
extern uint64_t walk_stack(uint64_t *array, uint64_t max);

/// is fully loaded yet.
uint32_t bootspace = 2;

uint8_t kerror_mode = 0;

struct term_context *term_context;

static struct PageTable *test_table;

uint8_t init_done = 0;

HANDLE kernel_heap = NULL;

static uint64_t supply_from_early(void)
{
    return early_alloc_page();
}
static uint64_t supply_from_frame(void)
{
    return frame_alloc();
}

extern volatile struct limine_hhdm_request   hhdm_request;
extern volatile struct limine_memmap_request memmap_req;

static void limine_debug(void)
{
    // HHDM
    if (hhdm_request.response == NULL)
    {
        printf_("[LIMINE] HHDM request: NULL\n");
    }
    else
    {
        printf_("[LIMINE] HHDM offset: 0x%llx\n",
                (unsigned long long) hhdm_request.response->offset);
    }

    // Kernel addresses
    if (Kaddress_req.response == NULL)
    {
        printf_("[LIMINE] Kernel address request: NULL\n");
    }
    else
    {
        printf_("[LIMINE] Kernel phys base: 0x%llx virt base: 0x%llx\n",
                (unsigned long long) Kaddress_req.response->physical_base,
                (unsigned long long) Kaddress_req.response->virtual_base);
    }

    // Memory map
    if (memmap_req.response == NULL)
    {
        printf_("[LIMINE] Memory map request: NULL\n");
    }
    else
    {
        printf_("[LIMINE] Memmap entries: %llu\n",
                (unsigned long long) memmap_req.response->entry_count);
    }
}

static void memory_bringup(void)
{
    // 1) Tiny bump allocator online

    printf_("%s\n", "Bootstrapping Memory... calling bootstrap alloc... ");

    early_init();

    printf_("%s\n", "Bootstrapping Memory... getting frames... ");

    // 2) During paging bootstrap, page tables need frames from EARLY
    g_boot_phys_page = supply_from_early;

    // Your existing paging init runs unchanged; it calls PagingGetFreeFrame()
    // which now routes to early_alloc_page().
    // If your entry point is different, call that instead.
    // e.g. paging_init() / paging_early_bootstrap() / etc.
    // paging_early_bootstrap(...)  // if you have a bootstrap function
    // OR:
    // whatever you already call to build PML4 → PDPT → PD → PT

    printf_("%s\n",
            "Bootstrapping Memory... building PML4 -> PDPT -> PD -> PT... ");

    limine_debug();

    paging_bootstrap();

    printf_("CR3 after bootstrap: 0x%llx\n", ReadCR3());

    // 3) Now the real bitmap allocator; it allocates its own bitmaps via early
    // pages
    frame_init();

    // 4) From now on, all callers of PagingGetFreeFrame() get frames from the
    // proper allocator
    g_boot_phys_page = supply_from_frame;
}

void proc_a(void)
{
    while (1)
    {
        printf_("%s\n", "I'm a thread");
        for (volatile int i = 0; i < 1000000; i++)
            ; // simple delay
    }
}

void proc_b(void)
{
    while (1)
    {
        printf_("%s\n", "I'm a catgirl");
        for (volatile int i = 0; i < 1000000; i++)
            ; // simple delay
    }
}

void hello_general_floatius()
{
    double t;

    double x = 5.239;

    t = 5 / 2;

    x = t * 6.4;

    printf_("%g\n", t);
    printf_("%g\n", x);
}

void _start(void)
{
    if (early_term.response == NULL || early_term.response->terminal_count < 1)
    {
        bootspace = 1;

        printf_("%s\n",
                "WARNING: Bootloader Terminal Offline Using Serial Only!");
    }

    printf_("%s", "Early Terminal Using Framebuffer At Physical Address: ");
    printf_("0x%llx\n",
            TranslateToPhysicalMemoryAddress(
                early_term.response->terminals[0]->framebuffer));
    printf_("%s", "And At Virtual Address: ");
    printf_("0x%llx\n", early_term.response->terminals[0]->framebuffer);

    print_stack_size();

    print_date();

    cpuid_readout();

    if (k_mode.hw_rng_support == 1)
    {
        printf_("%s", "Random Number Gen (HW) Test: ");
        printf_("%u\n", rand_asm());
    }
    // breakpoint();

    stop_interrupts();

    LoadGDT_Stage1();

    printf_("%s\n", "Loaded GDT");

    // breakpoint();

    idt_init();

    printf_("%s\n", "Loaded IDT");

    pic_enable();

    printf_("%s\n", "PICs Online");

    time_init();

    asm volatile("cli");

    // lapic_init();

    asm volatile("sti");

    asm volatile("cli");

    idt_reload();

    gdt_reload();

    asm volatile("sti");

    idt_reg_test();

    asm volatile("int $48");

    yield_register();

    asm volatile("int $49");

    pic_mask_irq(0);

    // print_memmap();

    memory_bringup();

    kmalloc_init();

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

    // breakpoint();

    // read_memory_map();

    // print_memory();

    // init_memory();

    printf_("%s", "CR3: ");
    printf_("0x%llx\n", readCR3());
    printf_("%s", "CR0: ");
    printf_("0x%llx\n", readCRO());
    printf_("%s", "CR4: ");
    printf_("0x%llx\n", readCR4());

    kernel_heap = pmalloc_init(0x2FAF080);

    if (k_mode.acpi_support == 1)
    {
        acpi_init();
    }

    keyboard_init();

    printf_("%s\n", "Handing Control to Standalone Terminal...");

    bootspace = 3;

    for (uint64_t i = 0; i < 500; i++)
    {
        printf_("%s\n", "");
    }

    early_term.response->write(
        early_term.response->terminals[0], NULL, LIMINE_TERMINAL_FULL_REFRESH);

    bootspace = 1;

    term_context = fbterm_init(kmalloc,
                               fbr_req.response->framebuffers[0]->address,
                               fbr_req.response->framebuffers[0]->width,
                               fbr_req.response->framebuffers[0]->height,

                               fbr_req.response->framebuffers[0]->pitch,
                               NULL,
                               NULL,
                               NULL,
                               &term_bg,
                               &term_fg,
                               &vgafont,
                               8,
                               16,
                               1,

                               1,
                               1,
                               1);

    bootspace = 0;

    // VMM_table_clone();

    pic_unmask_irq(0);

    // cpuid_readout();

    // print_memory();

    print_load_time();

    print_date();

    print_stack_size();

    printf_("%s\n", "VoyagerOS64 v0.0.4");

    printf_("%s\n", ":> ");

    init_done = 1;

    // hello_general_floatius();

    bootspace = 1;

    if (k_mode.addr_debug == 1)
    {
        print_frame_bitmap();
    }

    bootspace = 0;

    // stack_dump_asm();

    // halt();

    uint64_t loopcount = 0;
    // Just chill until needed

    // pic_mask_irq(0);

    init_scheduler();

    create_process(proc_a);

    create_process(proc_b);

    // pic_unmask_irq(0);

    while (1)
    {
    }
}