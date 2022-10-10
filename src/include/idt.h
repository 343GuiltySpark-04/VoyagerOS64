#pragma once
#include <stdint.h>
#include "global_defs.h"

#define IDT_INTERRUPT_GATE 0xE
#define IDT_TRAP_GATE 0xF
#define IDT_ENTRIES 256

struct PACKED IDTDescEntry
{
    uint16_t ptrLow;
    uint16_t selector;
    uint8_t ist;

    union
    {
        uint8_t flags;

        struct
        {
            uint8_t type : 4;
            uint8_t s : 1;
            uint8_t dpl : 2;
            uint8_t present : 1;
        };
    };

    uint16_t ptrMid;
    uint32_t ptrHigh;
    uint32_t reserved;
};
