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

// The following will be our kernel's entry point.
void _start(void)
{
    breakpoint();

    stop_interrupts();

    LoadGDT_Stage1();

    serial_print_line("Loaded GDT");

    breakpoint();

    idt_init();

    serial_print_line("Loaded idt");

    printf_("%s\n", "Kernel Loaded");
    while (1)
    {
    }
}