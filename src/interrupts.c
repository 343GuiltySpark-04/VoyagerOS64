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
#include "include/stack_trace.h"
#include "include/KernelUtils.h"

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

void isr_exception_handler(isr_xframe_t *frame, uint64_t rsi);
/**
 * @brief This is the ISR handler.
 * @param * frame
 */
void isr_exception_handler(isr_xframe_t *frame, uint64_t rsi)
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
    printf_("%s", "CR4 REGISTER: ");
    printf_("0x%llx\n", frame->control_registers.cr4);
    printf_("%s", "RSP REGISTER: ");
    printf_("0x%lx\n", frame->base_frame.rsp);
    printf_("%s", "RFLAGS REGISTER: ");
    printf_("0x%llx\n", frame->base_frame.rflags);

    if (k_mode.stack_trace_on_fault == 1)
    {

        printf_("%s\n", "NOTE: Will Page Fault if frames run out before the 5 frame cap, this is harmless dissregard it and the resulting additonal trace.");
        stack_trace(0);
    }
    else if (k_mode.stack_trace_on_fault == 2)
    {

        stack_dump_asm();
        // stack_dump();
        // stack_dump_recursive(16);
    }

    __asm__ volatile("cli; hlt");
}

extern void halt();
extern void dyn_isr_handler(uint64_t isr);

void irq_handler(isr_xframe_t *frame);
/**
 * @brief The handler for IRQs.
 * @param * frame
 */
void irq_handler(isr_xframe_t *frame)
{

    uint64_t vector = frame->base_frame.vector;

    if (vector < 48)
    {
        switch (vector)
        {
        case 32:
            sys_clock_handler_alt();
            break;
        case 33:
            keyboard_handler();
            break;
        }
    }
    else
    {

        printf_("IRQ Handoff to Dynamic Handler Using Vector: %i With Handler Located At Address: 0x%llx\n", vector, isr_delta[vector]);

        if (isr_delta[vector] == NULL)
        {

            printf_("%s\n", "Kernel Panic: Invalid/NULL Dynamic ISR Vector!");
            halt();
        }
        else
        {

            dyn_isr_handler(isr_delta[vector]);
        }
    }

    pic_send_eoi(vector - 32);
}
