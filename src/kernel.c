/**
 * Copyright (c) 2025 Tristan Adams
 *
 * This software is released under the MIT License.
 * https://opensource.org/licenses/MIT
 */

#include "include/KernelUtils.h"
#include "include/acpi/acpi.h"
#include "include/apic/lapic.h"
#include "include/consts.h"
#include "include/cpuUtils.h"
#include "include/drivers/keyboard/keyboard.h"
#include "include/early_alloc.h"
#include "include/gdt.h"
#include "include/idt.h"
#include "include/io.h"
#include "include/kernel.h"
#include "include/limine.h"
#include "include/mm/kmalloc.h"
#include "include/paging/frame_supplier.h"
#include "include/paging/neo_framealloc.h"
#include "include/paging/paging.h"
#include "include/paging/paging_bootstrap.h"
#include "include/panic.h"
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

extern volatile struct limine_kernel_address_request Kaddress_req;
extern volatile struct limine_terminal_request       early_term;
extern volatile struct limine_framebuffer_request    fbr_req;

extern void     breakpoint(void);
extern void     stop_interrupts(void);
extern void     start_interrupts(void);
extern void     halt(void);
extern void     task_switch_int(void);
extern uint64_t walk_stack(uint64_t *array, uint64_t max);

uint32_t             bootspace   = 2;
uint8_t              kerror_mode = 0;
struct term_context *term_context;
uint8_t              init_done = 0;

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
    if (hhdm_request.response == NULL)
    {
        printf_("[LIMINE] HHDM request: NULL\n");
    }
    else
    {
        printf_("[LIMINE] HHDM offset: 0x%llx\n",
                (unsigned long long) hhdm_request.response->offset);
    }

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
    printf_("%s\n", "Bootstrapping Memory... calling bootstrap alloc... ");
    early_init();

    printf_("%s\n", "Bootstrapping Memory... getting frames... ");
    g_boot_phys_page = supply_from_early;

    printf_("%s\n",
            "Bootstrapping Memory... building PML4 -> PDPT -> PD -> PT... ");
    limine_debug();
    paging_bootstrap();

    printf_("CR3 after bootstrap: 0x%llx\n", readCR3());

    frame_init();
    g_boot_phys_page = supply_from_frame;
}

static void scheduler_heartbeat(const char *message)
{
    while (*message)
        serial_debug(*message++);
    serial_debug('\n');
}

void proc_a(void)
{
    while (1)
    {
        scheduler_heartbeat("I'm a thread");
        for (volatile int i = 0; i < 1000000; i++)
            ;
        schedule();
    }
}

void proc_b(void)
{
    while (1)
    {
        scheduler_heartbeat("I'm a catgirl");
        for (volatile int i = 0; i < 1000000; i++)
            ;
        schedule();
    }
}

void hello_general_floatius(void)
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
                (uint64_t) (uintptr_t) early_term.response->terminals[0]
                    ->framebuffer));
    printf_("%s", "And At Virtual Address: ");
    printf_(
        "0x%llx\n",
        (uint64_t) (uintptr_t) early_term.response->terminals[0]->framebuffer);

    print_stack_size();
    print_date();
    cpuid_readout();

    if (k_mode.hw_rng_support == 1)
    {
        printf_("%s", "Random Number Gen (HW) Test: ");
        printf_("%u\n", rand_asm());
    }

    stop_interrupts();
    LoadGDT_Stage1();
    printf_("%s\n", "Loaded GDT");

    idt_init();
    printf_("%s\n", "Loaded IDT");

    pic_enable();
    printf_("%s\n", "PICs Online");

    time_init();

    asm volatile("cli");
    asm volatile("sti");
    asm volatile("cli");

    idt_reload();
    gdt_reload();

    asm volatile("cli");

    idt_reg_test();
    asm volatile("int $48");

    yield_register();
    asm volatile("int $49");

    memory_bringup();
    kmalloc_init();

    /* Keep PIT preemption parked during the cooperative 0.0.5 bring-up. */
    pic_mask_irq(0);
    asm volatile("sti");

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

    printf_("%s", "CR3: ");
    printf_("0x%llx\n", readCR3());
    printf_("%s", "CR0: ");
    printf_("0x%llx\n", readCRO());
    printf_("%s", "CR4: ");
    printf_("0x%llx\n", readCR4());

    if (k_mode.acpi_support == 1)
        acpi_init();

    printf_("%s\n", "Handing Control to Standalone Terminal...");

    /* Once Voyager owns CR3, Limine's terminal callback is no longer safe.
     * Stay on serial while fbterm takes ownership of the framebuffer. */
    bootspace = 1;

    if (fbr_req.response == NULL || fbr_req.response->framebuffer_count < 1)
        panic("Framebuffer terminal requested without a framebuffer");

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
                               NULL,
                               0,
                               0,
                               0,
                               1,
                               1,
                               1);

    if (!term_context)
        panic("Framebuffer terminal initialization failed");

    if (term_context->clear)
        term_context->clear(term_context, true);
    if (term_context->full_refresh)
        term_context->full_refresh(term_context);

    bootspace = 0;
    keyboard_init();

    pic_unmask_irq(0);

    print_load_time();
    // print_stack_size();
    print_date();

    printf_("%s", "Lucy ");
    printf_("%s\n", KERNEL_VERSION);

    init_done = 1;

    bootspace = 1;
    if (k_mode.addr_debug == 1)
    {
        printf_("Neo PMM frames: total=%llu free=%llu\n",
                (unsigned long long) frame_total_count(),
                (unsigned long long) frame_free_count());
    }
    bootspace = 0;

    init_scheduler();
    // create_process(proc_a);
    //  create_process(proc_b);
    create_process(vsh_loop);

    // printf_("%s\n", "Scheduler cooperative round-robin online.");
    scheduler_start();

    panic("scheduler returned control to kernel bootstrap");
}
