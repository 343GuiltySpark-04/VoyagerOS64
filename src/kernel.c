#include <stdint.h>
#include <stddef.h>
#include "include/limine.h"
#include "include/gdt.h"
#include "include/idt.h"
#include "include/KernelUtils.h"
#include "include/serial.h"
#include "include/printf.h"

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

extern void breakpoint();
extern void stop_interrupts();
extern void start_interrupts();

static volatile struct limine_terminal_request terminal_request = {
    .id = LIMINE_TERMINAL_REQUEST,
    .revision = 0};

static volatile struct limine_memmap_request memmap_request =
    {
        .id = LIMINE_MEMMAP_REQUEST, .revision = 0};

static void done(void)
{
    for (;;)
    {
        __asm__("hlt");
    }
}

// The following will be our kernel's entry point.
void _start(void)
{
    breakpoint();

    printf("t%stest");

    stop_interrupts();

    LoadGDT_Stage1();

    serial_print_line("Loaded GDT");

    breakpoint();

    idt_init();

    serial_print_line("Loaded idt");

    // Ensure we got a terminal
    if (terminal_request.response == NULL || terminal_request.response->terminal_count < 1)
    {
        done();
    }

    // breakpoint();

    struct limine_memmap_response *memory_map_response = memmap_request.response;

    // We should now be able to call the Limine terminal to print out
    // a simple "Hello World" to screen.
    struct limine_terminal *terminal = terminal_request.response->terminals[0];
    terminal_request.response->write(terminal, "VoyagerOS64, Version 0.0.1", 26);

    // We're done, just hang...
    // breakpoint();
    while (1)
    {
    }
}