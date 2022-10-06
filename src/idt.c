#include "include/global_defs.h"
#include <stdint.h>
#include "include/idt.h"

__attribute__((aligned(0x10))) static idt_entry_t idt[256]; // Create an array of IDT entries; aligned for performance
typedef struct
{
    uint16_t base_low;   // The lower 16 bits of the ISR's address
    uint16_t cs; // The GDT segment selector that the CPU will load into CS before calling the ISR
    uint8_t ist;        // The IST in the TSS that the CPU will load into RSP; set to zero for now
    uint8_t attributes; // Type and attributes; see the IDT page
    uint16_t base_mid;   // The higher 16 bits of the lower 32 bits of the ISR's address
    uint32_t isr_high;  // The higher 32 bits of the ISR's address
    uint32_t reserved;  // Set to zero
} PACKED idt_entry_t;

typedef struct
{
    uint16_t limit;
    uint64_t base;
} PACKED idtr_t;


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
    descriptor->rsv0 = 0;
}