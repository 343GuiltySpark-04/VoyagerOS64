#include <stdint.h>
#include "include/string.h"
#include <stdbool.h>
#include "include/global_defs.h"
#include "include/idt.h"
#include "include/printf.h"
#include "include/gdt.h"

extern void halt();

ALIGN_16BIT static idt_entry_t idt[IDT_ENTRIES];

static idtr_t idtr;

static bool vectors[IDT_ENTRIES];

void int_handler()
{

    printf_("%s\n", "!!!KERNEL PANIC!!!");
    printf_("%s", "INTERRUPT HANDLING NOT AVIAL!");
}

void irq_handler()
{

    printf_("%s\n", "!!!KERNEL PANIC!!!");
    printf_("%s", "INTERRUPT HANDLING NOT AVIAL!");
}

void exception_handler()
{

    printf_("%s\n", "!!!KERNEL PANIC!!!");
    printf_("%s", "INTERRUPT HANDLING NOT AVIAL!");

    halt();
}

void idt_set_descriptor(uint8_t vector, void *isr, uint8_t flags)
{
    idt_entry_t *descriptor = &idt[vector];

    descriptor->base_low = (uint64_t)isr & 0xFFFF;
    descriptor->cs = GDTKernelBaseSelector;
    descriptor->ist = 0;
    descriptor->attributes = flags;
    descriptor->base_mid = ((uint64_t)isr >> 16) & 0xFFFF;
    descriptor->base_high = ((uint64_t)isr >> 32) & 0xFFFFFFFF;
    descriptor->rsv0 = 0;
}

extern void *isr_stub_table[];

void idt_init()
{
    idtr.base = (uintptr_t)&idt[0];
    idtr.limit = (uint16_t)sizeof(idt_entry_t) * IDT_ENTRIES - 1;

    for (uint8_t vector = 0; vector < 32; vector++)
    {
        idt_set_descriptor(vector, isr_stub_table[vector], 0x8E);
        vectors[vector] = true;
    }

    __asm__ volatile("lidt %0"
                     :
                     : "m"(idtr)); // load the new IDT
    __asm__ volatile("sti");       // set the interrupt flag
}