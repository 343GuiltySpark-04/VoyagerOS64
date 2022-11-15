#pragma once

#include <stdint.h>
#include "global_defs.h"
#include "tss.h"

#define GDTAccessDPL(n) (n << 5)

enum GDTAccessFlag
{
    ReadWrite = (1 << 1),
    DC = (1 << 2),
    Execute = (1 << 3),
    Segments = (1 << 4),
    Present = (1 << 7),
};

#define GDTKernelBaseSelector 0x28
#define GDTUserBaseSelector 0x48
#define GDTTSSSegment 0x50

#define GDTAccess16Code (ReadWrite | Execute | Segments | Present)
#define GDTAccess16Data (ReadWrite | Segments | Present)
#define GDTAccess32Code (ReadWrite | Execute | Segments | Present)
#define GDTAccess32Data (ReadWrite | Segments | Present)
#define GDTAccessKernelCode (ReadWrite | Execute | Segments | Present)
#define GDTAccessKernelData (ReadWrite | Segments | Present)
#define GDTAccessUserCode (ReadWrite | Execute | Segments | GDTAccessDPL(3) | Present)
#define GDTAccessUserData (ReadWrite | Segments | GDTAccessDPL(3) | Present)

typedef struct PACKED GDT_Desc
{
    uint16_t size;
    uint64_t offset;
};

typedef struct PACKED GDT_Entry
{
    uint16_t limit_low;
    uint16_t base_low;
    uint8_t base_middle;
    uint8_t access_flag;
    uint8_t limit_flags;
    uint8_t base_high;
};

typedef struct PACKED GDT_Entry_16
{

    uint16_t limit_low;
    uint16_t base_low;
    uint8_t base_middle;
    uint8_t access_flag;
    uint8_t limit_flags;
    uint8_t base_high;
};

typedef struct PACKED GDT_Entry_32
{

    uint16_t limit_low;
    uint16_t base_low;
    uint8_t base_middle;
    uint8_t access_flag;
    uint8_t limit_flags;
    uint8_t base_high;
};

typedef struct PACKED TSS_Entry
{
    uint16_t length;
    uint16_t base_low;
    uint8_t base_mid;
    uint8_t flags;
    uint8_t flags2;
    uint8_t base_high;
    uint32_t base_up;
    uint32_t reserved0;
};

struct PACKED ALIGN_4K GDT
{
    struct GDT_Entry null;
    struct GDT_Entry_16 seg_16_code;
    struct GDT_Entry_16 seg_16_data;
    struct GDT_Entry_32 seg_32_code;
    struct GDT_Entry_32 seg_32_data;
    struct GDT_Entry kernelCS;
    struct GDT_Entry kernelData;
    struct GDT_Entry userNull;
    struct GDT_Entry userData;
    struct GDT_Entry userCode;
    struct TSS_Entry tss;
};

extern uint64_t rsp0;

void LoadGDT_Stage1();

void gdt_load_tss(struct TSS *tss);

void gdt_reload(void);
