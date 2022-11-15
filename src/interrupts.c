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
#include "include/time.h"
#include "include/kernel.h"
#include "include/sched.h"

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

    kerror_mode = 1;

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

extern void halt();
extern void dyn_isr_handler(uint64_t isr);

void irq_handler(isr_xframe_t *frame);
void irq_handler(isr_xframe_t *frame)
{

    uint64_t vector = frame->base_frame.vector;

    // printf_("%s", "IRQ RECIVED FROM: ");
    // printf_("%s\n", irq_messages[vector - 32]);

    if (vector < 48)
    {
        switch (vector)
        {

        case 32:
            sys_clock_handler();
            break;
        case 33:
            keyboard_handler();
            break;
        }
    }
    else
    {

        printf_("%s", "IRQ Handoff to Dynamic Handler Using Vector: ");
        printf_("%i", vector);
        printf_("%s", " With Handler Located At Address: ");
        printf_("0x%llx\n", isr_delta[vector]);

        if (isr_delta[vector] == NULL)
        {

            printf_("%s\n", "Needs More Work!");
            halt();
        }
        else
        {

            dyn_isr_handler(isr_delta[vector]);
        }
    }

    pic_send_eoi(vector - 32);
}