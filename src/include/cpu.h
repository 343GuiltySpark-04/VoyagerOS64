/**
 * @file cpu.h
 * @brief Low-level CPU context, feature, and register-state helpers.
 * @ingroup boot
 */
#pragma once

#ifndef _CPU_H
#define _CPU_H

#include "registers.h"
#include "tss.h"
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

extern bool sysenter;

struct thread;

/** @brief Saved general-purpose CPU context used by interrupt/APIC groundwork. */
struct cpu_ctx
{
    uint64_t ds;
    uint64_t es;
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
    uint64_t err;
    uint64_t rip;
    uint64_t cs;
    uint64_t rflags;
    uint64_t rsp;
    uint64_t ss;
};

/** @brief Per-CPU state reserved for APIC/SMP development. */
struct cpu_local
{
    int            cpu_number;
    bool           bsp;
    bool           active;
    int            last_run_queue_index;
    uint32_t       lapic_id;
    uint32_t       lapic_freq;
    struct TSS     tss;
    struct thread *idle_thread;
    void (*timer_function)(int, struct cpu_ctx *);
};

void cpu_init(void);

extern size_t fpu_bank_size;
extern void (*fpu_save)(void *ctx);
extern void (*fpu_rest)(void *ctx);

/**
 * @brief Save enabled extended processor state with XSAVE.
 * @param ctx Destination buffer satisfying the alignment/size requirements for
 *            the configured XSAVE state.
 *
 * @warning The 0.0.5 scheduler does not yet maintain a validated per-task XSAVE
 * state area. These helpers are groundwork, not part of qualified context
 * switching.
 */
static inline void xsave(void *ctx)
{
    asm volatile("xsave (%0)"
                 :
                 : "r"(ctx), "a"(0xffffffff), "d"(0xffffffff)
                 : "memory");
}

/**
 * @brief Restore enabled extended processor state with XRSTOR.
 * @param ctx Source buffer containing a compatible XSAVE state image.
 */
static inline void xrstor(void *ctx)
{
    asm volatile("xrstor (%0)"
                 :
                 : "r"(ctx), "a"(0xffffffff), "d"(0xffffffff)
                 : "memory");
}

/**
 * @brief Save legacy x87/SSE state with FXSAVE.
 * @param ctx Destination buffer suitable for FXSAVE.
 */
static inline void fxsave(void *ctx)
{
    asm volatile("fxsave (%0)" : : "r"(ctx) : "memory");
}

/**
 * @brief Restore legacy x87/SSE state with FXRSTOR.
 * @param ctx Source buffer containing a compatible FXSAVE image.
 */
static inline void fxrstor(void *ctx)
{
    asm volatile("fxrstor (%0)" : : "r"(ctx) : "memory");
}

/**
 * @brief Test the current RFLAGS interrupt-enable bit.
 * @return true when maskable interrupts are enabled (RFLAGS.IF is set).
 */
static inline bool interrupt_state(void)
{
    uint64_t flags;
    asm volatile("pushfq; pop %0" : "=rm"(flags));
    return flags & (1 << 9);
}

/**
 * @brief Write a 64-bit value to an MSR.
 * @param msr Address/index of the MSR to write.
 * @param val Value to write.
 * @return The value assembled from the EDX:EAX inputs used by WRMSR.
 */
static inline uint64_t wrmsr(uint32_t msr, uint64_t val)
{
    uint32_t eax = (uint32_t) val;
    uint32_t edx = (uint32_t) (val >> 32);
    asm volatile("wrmsr\n\t" : : "a"(eax), "d"(edx), "c"(msr) : "memory");
    return ((uint64_t) edx << 32) | eax;
}

/**
 * @brief Read the processor time-stamp counter.
 * @return Value assembled by the current RDTSC helper implementation.
 */
static inline uint64_t rdtsc(void)
{
    uint32_t edx, eax;
    asm volatile("rdtsc" : "=d"(edx), "=a"(eax));
    return ((uint64_t) edx << 32) | edx;
}

/**
 * @brief Execute RDRAND and return the produced 64-bit value.
 * @return Value written by the RDRAND instruction.
 *
 * @note This wrapper does not expose the instruction's carry-flag success
 * indication. Callers must not infer successful entropy generation solely from
 * the existence of this helper.
 */
static inline uint64_t rdrand(void)
{
    uint64_t result;
    asm volatile("rdrand %0" : "=r"(result));
    return result;
}

/**
 * @brief Execute RDSEED and return the produced 64-bit value.
 * @return Value written by the RDSEED instruction.
 *
 * @note This wrapper does not expose the instruction's carry-flag success
 * indication.
 */
static inline uint64_t rdseed(void)
{
    uint64_t result;
    asm volatile("rdseed %0" : "=r"(result));
    return result;
}

#define CPUID_XSAVE ((uint32_t) 1 << 26)
#define CPUID_AVX ((uint32_t) 1 << 28)
#define CPUID_AVX512 ((uint32_t) 1 << 16)
#define CPUID_SEP ((uint32_t) 1 << 11)

/**
 * @brief Execute CPUID for one leaf/subleaf when the leaf is supported.
 * @param leaf CPUID leaf number.
 * @param subleaf CPUID subleaf number supplied in ECX.
 * @param eax Output storage for EAX.
 * @param ebx Output storage for EBX.
 * @param ecx Output storage for ECX.
 * @param edx Output storage for EDX.
 * @return true when the requested leaf passes the helper's maximum-leaf check;
 *         false otherwise.
 */
static inline bool cpuid(uint32_t  leaf,
                         uint32_t  subleaf,
                         uint32_t *eax,
                         uint32_t *ebx,
                         uint32_t *ecx,
                         uint32_t *edx)
{
    uint32_t cpuid_max;
    asm volatile("cpuid"
                 : "=a"(cpuid_max)
                 : "a"(leaf & 0x80000000)
                 : "rbx", "rcx", "rdx");
    if (leaf > cpuid_max)
    {
        return false;
    }
    asm volatile("cpuid"
                 : "=a"(*eax), "=b"(*ebx), "=c"(*ecx), "=d"(*edx)
                 : "a"(leaf), "c"(subleaf));
    return true;
}

/**
 * @brief Set IA32_KERNEL_GS_BASE.
 * @param addr Address to install as the kernel GS base.
 */
static inline void set_kernel_gs_base(void *addr)
{
    wrmsr(0xc0000102, (uint64_t) addr);
}

/**
 * @brief Set IA32_GS_BASE.
 * @param addr Address to install as the GS base.
 */
static inline void set_gs_base(void *addr)
{
    wrmsr(0xc0000101, (uint64_t) addr);
}

/**
 * @brief Set IA32_FS_BASE.
 * @param addr Address to install as the FS base.
 */
static inline void set_fs_base(void *addr)
{
    wrmsr(0xc0000100, (uint64_t) addr);
}

/**
 * @brief Read IA32_KERNEL_GS_BASE.
 * @return Current kernel GS base address.
 */
static inline void *get_kernel_gs_base(void)
{
    return (void *) rdmsr(0xc0000102);
}

/**
 * @brief Read IA32_GS_BASE.
 * @return Current GS base address.
 */
static inline void *get_gs_base(void)
{
    return (void *) rdmsr(0xc0000101);
}

/**
 * @brief Read IA32_FS_BASE.
 * @return Current FS base address.
 */
static inline void *get_fs_base(void)
{
    return (void *) rdmsr(0xc0000100);
}

/** @brief Enable maskable interrupts with STI. */
static inline void enable_interrupts(void)
{
    asm("sti");
}

/** @brief Disable maskable interrupts with CLI. */
static inline void disable_interrupts(void)
{
    asm("cli");
}

/**
 * @brief Set the maskable-interrupt enable state.
 * @param state true to enable interrupts; false to disable them.
 * @return Previous RFLAGS.IF state.
 */
static inline bool interrupt_toggle(bool state)
{
    bool ret = interrupt_state();
    if (state)
    {
        enable_interrupts();
    }
    else
    {
        disable_interrupts();
    }
    return ret;
}

/**
 * @brief Return the current CPU-local record when that facility is available.
 * @return Pointer to CPU-local state, or the implementation's current fallback.
 *
 * @note CPU-local/SMP state is not yet a qualified 0.0.5 scheduler facility.
 */
struct cpu_local *this_cpu(void);

#endif