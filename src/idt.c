#include "include/global_defs.h"
#include <stdint.h>
#include "include/idt.h"
#include <stdbool.h>

// Based on https://github.com/austanss/skylight/blob/trunk/glass/src/cpu/interrupts/idt.c

idt_entry_t idt[IDT_MAX_DESCRIPTORS];

static bool vectors[IDT_MAX_DESCRIPTORS];

static idtr_t idtr;

__attribute__((noreturn)) void exception_handler(void);
void exception_handler()
{
    __asm__ volatile("cli; hlt"); // Completely hangs the computer
}

void idt_set_descriptor(uint8_t vector, void *isr, uint8_t flags)
{
    idt_entry_t *descriptor = &idt[vector];

    descriptor->base_low = (uint64_t)isr & 0xFFFF;
    descriptor->cs = 0x0008;
    descriptor->ist = 0;
    descriptor->attributes = flags;
    descriptor->base_mid = ((uint64_t)isr >> 16) & 0xFFFF;
    descriptor->base_high = ((uint64_t)isr >> 32) & 0xFFFFFFFF;
    descriptor->reserved = 0;
}

extern void *isr_stub_table[];

void idt_init(void);
void idt_init()
{
    idtr.base = (uintptr_t)&idt[0];
    idtr.limit = (uint16_t)sizeof(idt_entry_t) * IDT_MAX_DESCRIPTORS - 1;

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