/**
 * @file tss.h
 * @brief x86-64 Task State Segment layout.
 * @ingroup interrupts
 */
#pragma once
#include "global_defs.h"
#include <stdint.h>

#ifndef _TSS_H
#define _TSS_H

/**
 * @brief Packed architectural x86-64 Task State Segment.
 *
 * The RSP fields provide privilege-level stack pointers and the IST fields
 * provide dedicated interrupt stacks when configured by the IDT. The I/O map
 * base occupies the final architectural field.
 *
 * @note The presence of a hardware TSS does not imply Voyager uses hardware
 * task switching; the 0.0.5 scheduler performs its own cooperative stack
 * switch in software.
 */
struct PACKED TSS
{
    uint32_t reserved0;
    uint64_t rsp0;
    uint64_t rsp1;
    uint64_t rsp2;
    uint64_t reserved1;
    uint64_t ist1;
    uint64_t ist2;
    uint64_t ist3;
    uint64_t ist4;
    uint64_t ist5;
    uint64_t ist6;
    uint64_t ist7;
    uint64_t reserved2;
    uint16_t reserved3;
    uint16_t iopbOffset;
};

#endif
