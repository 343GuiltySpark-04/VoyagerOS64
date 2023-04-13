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
#include "include/apic/lapic.h"
#include "include/stack_trace.h"
#include "include/io.h"
#include "include/pebble.h"
#include "include/panic.h"

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

extern void breakpoint();
extern void stop_interrupts();
extern void start_interrupts();
extern void halt();
extern void task_switch_int();
extern uint64_t walk_stack(uint64_t *array, uint64_t max);

/// is fully loaded yet.
uint32_t bootspace = 2;

uint8_t kerror_mode = 0;

struct term_context *term_context;

static struct PageTable *test_table;

struct Scheduler scheduler;

struct standby_tube k_standby_tube;

struct active_tube k_active_tube;

struct process_list k_process_list;

struct hot_tube hot_tube;

HANDLE kernel_heap = NULL;

void hello_general_floatius()
{

    double t;

    double x = 5.239;

    t = 5 / 2;

    x = t * 6.4;

    printf_("%g\n", t);
    printf_("%g\n", x);
}

void hello_thread()
{

    fork(&scheduler);

    printf_("%s\n", "Hello Im a Child Process!");

    return;
}

void _start(void)
{

    if (early_term.response == NULL || early_term.response->terminal_count < 1)
    {

        bootspace = 1;

        printf_("%s\n", "WARNING: Bootloader Terminal Offline Using Serial Only!");
    }

    printf_("%s", "Early Terminal Using Framebuffer At Physical Address: ");
    printf_("0x%llx\n", TranslateToPhysicalMemoryAddress(early_term.response->terminals[0]->framebuffer));
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

    print_memmap();

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

    //   print_memory();

    init_memory();

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

    early_term.response->write(early_term.response->terminals[0], NULL, LIMINE_TERMINAL_FULL_REFRESH);

    bootspace = 1;

    term_context = fbterm_init(vmalloc, fbr_req.response->framebuffers[0]->address, fbr_req.response->framebuffers[0]->width, fbr_req.response->framebuffers[0]->height,

                               fbr_req.response->framebuffers[0]->pitch, NULL, NULL, NULL, &term_bg, &term_fg, &vgafont, 8, 16, 1,

                               1, 1, 1);

    bootspace = 0;

    // VMM_table_clone();

    pic_unmask_irq(0);

    // cpuid_readout();

    //  print_memory();

    print_load_time();

    print_date();

    print_stack_size();

    printf_("%s\n", "VoyagerOS64 v0.0.4");

    printf_("%s\n", ":> ");

    // hello_general_floatius();

    bootspace = 1;

    init_sched(&k_standby_tube, &k_active_tube, &hot_tube);

    if (k_mode.addr_debug == 1)
    {
        print_frame_bitmap();
    }

    bootspace = 0;

    // stack_dump_asm();

    add_active_tube_process(&k_active_tube, create_tube_process(false, true, true, "Kernel_Thread"));
    add_tube_process(&k_standby_tube, create_tube_process(false, true, true, "Kernel_Thread_2"));
    add_tube_process(&k_standby_tube, create_tube_process(false, true, true, "Kernel_Thread_3"));

    active_pid = k_active_tube.processes[0].id;

    stdin("k", 123);

    // halt();

    uint64_t loopcount = 0;
    // Just chill until needed
    while (1)
    {

        if (loopcount == 1)
        {

            breakpoint();
            bootspace = 1;
        }

        tube_schedule(&k_standby_tube, &k_active_tube, &hot_tube, quantum);

        printf_("%s", "Loop number: ");
        printf_("%u\n", loopcount);
        printf_("%s", "Current PID: ");
        printf_("%u\n", k_active_tube.processes[0].id);
        printf_("%s", "Current Process Name: ");
        printf_("%s\n", k_active_tube.processes[0].name);
        printf_("%s", "Number of processes (Active and Standby Tubes): ");
        printf_("%u\n", k_active_tube.current_active + k_standby_tube.current_standby);
        printf_("%s", "Quantum Value of Current Process: ");
        printf_("%u\n", k_active_tube.processes[0].allocated_time);

        loopcount++;

        stdout("P", &k_active_tube, &k_standby_tube);

        if (loopcount >= 1)
        {

            temp = 1;
        }

        //  print_memory();

        if (loopcount == 25)
        {

            halt();
        }
    }
}