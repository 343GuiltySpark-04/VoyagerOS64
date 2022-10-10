#pragma once
#include <stdint.h>
#include <stdbool.h>
#include "global_defs.h"

#define IDT_INTERRUPT_GATE 0xE
#define IDT_TRAP_GATE 0xF
#define IDT_ENTRIES 256

typedef struct PACKED IDT_Entry
{

    uint16_t offset_low;
    uint16_t selector;
    uint8_t ist;

    union
    {

        struct
        {
            uint8_t type : 4;
            uint8_t s : 1;
            uint8_t dpl : 2;
            uint8_t present : 1;
        };
    };

    uint16_t offset_mid;
    uint32_t offset_high;
    uint32_t reserved;

} idt_entry_t;

uint8_t freevector;