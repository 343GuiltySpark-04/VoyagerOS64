#pragma once

#include "tss.h"
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

struct PACKED GDT_Desc
{
    uint16_t size;
    uint64_t offset;
};

struct PACKED GDT_Entry
{
    uint16_t base;
    uint16_t limit;
    uint8_t access_byte;
    uint8_t flags;
};


extern GDT_Desc;
extern GDT_Entry;

void LoadGDT();

