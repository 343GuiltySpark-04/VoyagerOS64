#include <stdint.h>
#include "include/global_defs.h"
#include "include/idt.h"
#include "include/printf.h"
#include "include/interrupts.h"
#include "include/io.h"
#include <stdbool.h>
#include "include/registers.h"
#include "include/pic.h"
#include "include/drivers/keyboard/keyboard.h"

uint64_t pic_map[16][16] = {};

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

static const char *irq_messages[] =

    {
        "System Timer",
        "Keyboard",
        "Slave PIC Link",
        "Serial Port 1",
        "Serial Port 2",
        "Reserved/Sound Card",
        "Floppy Disk Controller",
        "Parallel Port",
        "Real Time Clock",
        "Master PIC Link",
        "Reserved",
        "Reserved",
        "PS/2 Mouse",
        "Math Co-Processor",
        "Hard Disk Drive",
        "Reserved"

};

void isr_exception_handler(isr_xframe_t *frame);
void isr_exception_handler(isr_xframe_t *frame)
{
    printf_("%s", "ERROR: CPU EXCEPTION: ");
    printf_("%s", exception_messages[frame->base_frame.vector]);
    printf_("%s", " @ ");
    printf_("0x%llx\n", frame->base_frame.rip);
    printf_("%s", "FROM VECTOR NUMBER: ");
    printf_("%i\n", frame->base_frame.vector);
    printf_("%s", "ERROR CODE: ");
    printf_("0x%llx\n", frame->base_frame.error_code);
    printf_("%s", "CS REGISTER: ");
    printf_("0x%llx\n", frame->base_frame.cs);
    printf_("%s", "CR2 REGISTER: ");
    printf_("0x%llx\n", frame->control_registers.cr2);

    __asm__ volatile("cli; hlt");
}

void irq_handler(isr_xframe_t *frame);
void irq_handler(isr_xframe_t *frame)
{

    uint64_t vector = frame->base_frame.vector;

    // printf_("%s", "IRQ RECIVED FROM: ");
    // printf_("%s\n", irq_messages[vector - 32]);

    switch (vector)
    {

    case 32:
        sys_clock_handler();
        break;
    case 33:
        keyboard_handler();
        break;
    }

    pic_send_eoi(vector - 32);
}

void sys_clock_handler()
{

    printf_("%s\n", "Devs first handler fire!");
}
