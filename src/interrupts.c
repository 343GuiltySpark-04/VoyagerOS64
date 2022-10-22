#include <stdint.h>
#include "include/global_defs.h"
#include "include/idt.h"
#include "include/printf.h"
#include "include/interrupts.h"
#include "include/io.h"
#include <stdbool.h>
#include "include/registers.h"

static const char *exception_messages[] =
    {
        "Division By Zero",
        "Debug",
        "Non Maskable Interrupt",
        "Breakpoint",
        "Into Detected Overflow",
        "Out of Bounds",
        "Invalid Opcode",
        "No Coprocessor",

        "Double Fault",
        "Coprocessor Segment Overrun",
        "Bad TSS",
        "Segment Not Present",
        "Stack Fault",
        "General Protection Fault",
        "Page Fault",
        "Unknown Interrupt",

        "Coprocessor Fault",
        "Alignment Check",
        "Machine Check",
        "Reserved",
        "Reserved",
        "Reserved",
        "Reserved",
        "Reserved",

        "Reserved",
        "Reserved",
        "Reserved",
        "Reserved",
        "Reserved",
        "Reserved",
        "Reserved",
        "Reserved"};

void isr_exception_handler(isr_xframe_t *frame);
void isr_exception_handler(isr_xframe_t *frame)
{
    printf_("%s", "ERROR: CPU EXCEPTION: ");
    printf_("%s", exception_messages[frame->base_frame.vector]);
    printf_("%s", " @ ");
    printf_("0x%llx\n", frame->base_frame.rip);
    printf_("%s", "ERROR CODE: ");
    printf_("%i\n", frame->base_frame.error_code);
    printf_("%s", "CS REGISTER: ");
    printf_("0x%llx\n", frame->base_frame.cs);
    printf_("%s", "CR2 REGISTER: ");
    printf_("0x%llx\n", frame->control_registers.cr2);

    __asm__ volatile("cli; hlt");
}