/**
 * @file interrupts.h
 * @brief Register snapshot passed through Voyager's interrupt-dispatch path.
 * @ingroup interrupts
 */
#pragma once
#include "global_defs.h"
#include <stdint.h>

/**
 * @brief Extended interrupt frame matching the current ISR assembly/C boundary.
 *
 * The layout is an ABI between low-level interrupt stubs and C handlers. It
 * contains captured control registers, selected general-purpose registers, and
 * the architectural interrupt-return frame. Do not reorder fields without
 * updating the assembly that constructs/consumes this object.
 *
 * @note This interrupt frame is distinct from the cooperative scheduler's
 * saved context. VoyagerOS64 0.0.5 does not switch tasks by abandoning this
 * frame inside a hardware IRQ.
 */
typedef struct
{
    struct
    {
        uint64_t cr4;
        uint64_t cr3;
        uint64_t cr2;
        uint64_t cr0;
    } control_registers;

    struct
    {
        uint64_t rdi;
        uint64_t rsi;
        uint64_t rdx;
        uint64_t rcx;
        uint64_t rbx;
        uint64_t rax;
    } general_registers;

    struct
    {
        uint64_t rbp;
        uint64_t vector;
        uint64_t error_code;
        uint64_t rip;
        uint64_t cs;
        uint64_t rflags;
        uint64_t rsp;
        uint64_t dss;
    } base_frame;
} isr_xframe_t;
