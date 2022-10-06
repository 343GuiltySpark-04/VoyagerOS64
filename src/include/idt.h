#pragma once

#include <stdint.h>
#include "global_defs.h"

// Based on https://github.com/austanss/skylight/blob/trunk/glass/src/cpu/interrupts/idt.h

#define IDT_MAX_DESCRIPTORS 256

typedef struct
{
    uint16_t base_low;  // The lower 16 bits of the ISR's address
    uint16_t cs;        // The GDT segment selector that the CPU will load into CS before calling the ISR
    uint8_t ist;        // The IST in the TSS that the CPU will load into RSP; set to zero for now
    uint8_t attributes; // Type and attributes; see the IDT page
    uint16_t base_mid;  // The higher 16 bits of the lower 32 bits of the ISR's address
    uint32_t base_high; // The higher 32 bits of the ISR's address
    uint32_t reserved;  // Set to zero
} PACKED idt_entry_t;

typedef struct
{
    uint16_t limit;
    uint64_t base;
} PACKED idtr_t;

void idt_set_descriptor(uint8_t vector, void *isr, uint8_t flags);
void idt_init(void);