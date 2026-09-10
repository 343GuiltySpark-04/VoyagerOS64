/**
 * @file gdt.h
 * @brief x86-64 Global Descriptor Table and TSS descriptor interface.
 * @ingroup interrupts
 */
#pragma once

#include "global_defs.h"
#include "tss.h"
#include <stdint.h>

/** Encode a descriptor privilege level in a GDT access byte. */
#define GDTAccessDPL(n) (n << 5)

/** @brief Common access-byte flag bits used to build GDT entries. */
enum GDTAccessFlag
{
    ReadWrite = (1 << 1),
    DC        = (1 << 2),
    Execute   = (1 << 3),
    Segments  = (1 << 4),
    Present   = (1 << 7),
};

/** Kernel code-segment selector used by the current GDT layout. */
#define GDTKernelBaseSelector 0x28
/** User code-segment selector used by the current GDT layout. */
#define GDTUserBaseSelector 0x48
/** TSS descriptor selector used by the current GDT layout. */
#define GDTTSSSegment 0x50

#define GDTAccess16Code (ReadWrite | Execute | Segments | Present)
#define GDTAccess16Data (ReadWrite | Segments | Present)
#define GDTAccess32Code (ReadWrite | Execute | Segments | Present)
#define GDTAccess32Data (ReadWrite | Segments | Present)
#define GDTAccessKernelCode (ReadWrite | Execute | Segments | Present)
#define GDTAccessKernelData (ReadWrite | Segments | Present)
#define GDTAccessUserCode \
    (ReadWrite | Execute | Segments | GDTAccessDPL(3) | Present)
#define GDTAccessUserData (ReadWrite | Segments | GDTAccessDPL(3) | Present)

/** @brief Operand layout consumed by LGDT. */
struct PACKED GDT_Desc
{
    uint16_t size;
    uint64_t offset;
};

/** @brief Legacy-format 8-byte segment descriptor. */
struct PACKED GDT_Entry
{
    uint16_t limit_low;
    uint16_t base_low;
    uint8_t  base_middle;
    uint8_t  access_flag;
    uint8_t  limit_flags;
    uint8_t  base_high;
};

/** @brief Descriptor storage used for the 16-bit compatibility entries. */
struct PACKED GDT_Entry_16
{
    uint16_t limit_low;
    uint16_t base_low;
    uint8_t  base_middle;
    uint8_t  access_flag;
    uint8_t  limit_flags;
    uint8_t  base_high;
};

/** @brief Descriptor storage used for the 32-bit compatibility entries. */
struct PACKED GDT_Entry_32
{
    uint16_t limit_low;
    uint16_t base_low;
    uint8_t  base_middle;
    uint8_t  access_flag;
    uint8_t  limit_flags;
    uint8_t  base_high;
};

/** @brief 64-bit TSS system descriptor split into architectural fields. */
struct PACKED TSS_Entry
{
    uint16_t length;
    uint16_t base_low;
    uint8_t  base_mid;
    uint8_t  flags;
    uint8_t  flags2;
    uint8_t  base_high;
    uint32_t base_up;
    uint32_t reserved0;
};

/**
 * @brief Complete VoyagerOS64 GDT image.
 *
 * Field order is part of the selector ABI above: changing the order changes
 * descriptor offsets and therefore requires updating selector constants and
 * low-level reload code together.
 */
struct PACKED ALIGN_4K GDT
{
    struct GDT_Entry    null;
    struct GDT_Entry_16 seg_16_code;
    struct GDT_Entry_16 seg_16_data;
    struct GDT_Entry_32 seg_32_code;
    struct GDT_Entry_32 seg_32_data;
    struct GDT_Entry    kernelCS;
    struct GDT_Entry    kernelData;
    struct GDT_Entry    userNull;
    struct GDT_Entry    userData;
    struct GDT_Entry    userCode;
    struct TSS_Entry    tss;
};

/** Ring-0 stack pointer associated with the current TSS setup. */
extern uint64_t rsp0;

/** @brief Build/load the initial descriptor table used during boot. */
void LoadGDT_Stage1(void);
/** @brief Install/update the TSS descriptor for @p tss. */
void gdt_load_tss(struct TSS *tss);
/** @brief Reload segment/GDT state after table construction. */
void gdt_reload(void);
