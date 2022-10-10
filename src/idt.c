#include <stdint.h>
#include <string.h>
#include <stdbool.h>
#include "include/global_defs.h"
#include "include/idt.h"
#include "include/printf.h"
#include "include/gdt.h"

ALIGN_16BIT static idt_entry_t idt[IDT_ENTRIES];

static idtr_t idtr;

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