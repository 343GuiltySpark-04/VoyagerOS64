#pragma once

#include <stdint.h>
#include "global_defs.h"

#define GDTAccessDPL(n) (n << 5)

enum GDTAccessFlag
{
    ReadWrite = (1 << 1),
    DC = (1 << 2),
    Execute = (1 << 3),
    Segments = (1 << 4),
    Present = (1 << 7)
};

#define GDTKernelBaseSelector 0x08
#define GDTUserBaseSelector 0x18
#define GDTTSSSegment 0x30

#define GDTAccessKernelCode (ReadWrite | Execute | Segments | Present)
#define GDTAccessKernelData (ReadWrite | Segments | Present)
#define GDTAccessUserCode (ReadWrite | Execute | Segments | GDTAccessDPL(3) | Present)
#define GDTAccessUserData (ReadWrite | Segments | GDTAccessDPL(3) | Present)

typedef struct PACKED GDT_Entry
{
    uint16_t base;
    uint32_t limit;
    uint8_t access_byte;
    uint8_t flags;
} GDT_Entry_t;

typedef struct PACKED TSS_Entry
{

    uint64_t base;
    uint16_t limit;
    uint8_t access_byte;
    uint8_t flags;
    uint64_t rsp0;
    uint64_t ist1;
    uint64_t ist2;
} TSS_Entry_t;

typedef struct PACKED GDT
{

    GDT_Entry_t null_seg;

} GDT_t;

extern GDT;
extern GDT_Entry;
extern TSS_Entry;

void LoadGDT_Stage1();
