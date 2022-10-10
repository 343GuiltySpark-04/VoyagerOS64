#pragma once
#include <stdint.h>
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

struct PACKED cpu_state
{

    uint64_t rax;
    uint64_t rbx;
    uint64_t rcx;
    uint64_t rdx;
    uint64_t rsi;
    uint64_t rdi;
    uint64_t rbp;
    uint64_t r8;
    uint64_t r9;
    uint64_t r10;
    uint64_t r11;
    uint64_t r12;
    uint64_t r13;
    uint64_t r14;
    uint64_t r15;
};
