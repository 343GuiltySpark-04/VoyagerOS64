#pragma once

#ifndef _CPU_H
#define _CPU_H

#include "tss.h"
#include "registers.h"
#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

extern bool sysenter;

struct thread;

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

struct cpu_local
{

    int cpu_number;
    bool bsp;
    bool active;
    int last_run_queue_index;
    uint32_t lapic_id;
    uint32_t lapic_freq;
    struct TSS tss;
    struct thread *idle_thread;
    void (*timer_function)(int, struct cpu_ctx *);
};

void cpu_init(void);

extern size_t fpu_bank_size;
extern void (*fpu_save)(void *ctx);
extern void (*fpu_rest)(void *ctx);

/**
* @brief Save a context into x86.
* @param * ctx
* @return 0 on success non - zero on failure
*/
static inline void xsave(void *ctx)
{
    asm volatile("xsave (%0)"
                 :
                 : "r"(ctx), "a"(0xffffffff), "d"(0xffffffff)
                 : "memory");
}

/**
* @brief This function is used to reset the 64 - bit context.
* @param * ctx
* @return 0 on success non - zero on failure
*/
static inline void xrstor(void *ctx)
{
    asm volatile("xrstor (%0)"
                 :
                 : "r"(ctx), "a"(0xffffffff), "d"(0xffffffff)
                 : "memory");
}

/**
* @brief Save a context to memory.
* @param * ctx
* @return 0 on success non - zero
*/
static inline void fxsave(void *ctx)
{
    asm volatile("fxsave (%0)"
                 :
                 : "r"(ctx)
                 : "memory");
}

/**
* @brief Put a 64 - bit value into the FXRSTOR register.
* @param * ctx
*/
static inline void fxrstor(void *ctx)
{
    asm volatile("fxrstor (%0)"
                 :
                 : "r"(ctx)
                 : "memory");
}

/**
* @brief Check if we are in interrupt mode.
* @return true if we are in interrupt
*/
static inline bool interrupt_state(void)
{
    uint64_t flags;
    asm volatile("pushfq; pop %0"
                 : "=rm"(flags));
    return flags & (1 << 9);
}

/**
* @brief Write a 64 - bit value to MSR.
* @param msr Address of the MSR to write
* @param val Value to write to the MSR
* @return The value written to the
*/
static inline uint64_t wrmsr(uint32_t msr, uint64_t val)
{
    uint32_t eax = (uint32_t)val;
    uint32_t edx = (uint32_t)(val >> 32);
    asm volatile("wrmsr\n\t"
                 :
                 : "a"(eax), "d"(edx), "c"(msr)
                 : "memory");
    return ((uint64_t)edx << 32) | eax;
}

/**
* @brief Read 64 - bit timer counter
* @return The value of TSC
*/
static inline uint64_t rdtsc(void)
{
    uint32_t edx, eax;
    asm volatile("rdtsc"
                 : "=d"(edx), "=a"(eax));
    return ((uint64_t)edx << 32) | edx;
}

/**
* @brief Get a random number.
* @return A 64 - bit random number
*/
static inline uint64_t rdrand(void)
{
    uint64_t result;
    asm volatile("rdrand %0"
                 : "=r"(result));
    return result;
}

/**
* @brief Generate a 64 - bit random number.
* @return The 64 - bit random number
*/
static inline uint64_t rdseed(void)
{
    uint64_t result;
    asm volatile("rdseed %0"
                 : "=r"(result));
    return result;
}

#define CPUID_XSAVE ((uint32_t)1 << 26)
#define CPUID_AVX ((uint32_t)1 << 28)
#define CPUID_AVX512 ((uint32_t)1 << 16)
#define CPUID_SEP ((uint32_t)1 << 11)

/**
* @brief Run CPUID on a leaf
* @param leaf The leaf to process.
* @param subleaf The subleaf to process.
* @param * eax
* @param * ebx
* @param * ecx
* @param * edx
* @return True if the leaf is a processor
*/
static inline bool cpuid(uint32_t leaf, uint32_t subleaf,
                         uint32_t *eax, uint32_t *ebx, uint32_t *ecx, uint32_t *edx)
{
    uint32_t cpuid_max;
    asm volatile(
        "cpuid"
        : "=a"(cpuid_max)
        : "a"(leaf & 0x80000000)
        : "rbx", "rcx", "rdx");
    if (leaf > cpuid_max)
    {
        return false;
    }
    asm volatile(
        "cpuid"
        : "=a"(*eax), "=b"(*ebx), "=c"(*ecx), "=d"(*edx)
        : "a"(leaf), "c"(subleaf));
    return true;
}

/**
* @brief Set GS base address.
* @param * addr
* @return 0 on success non - zero on failure
*/
static inline void set_kernel_gs_base(void *addr)
{
    wrmsr(0xc0000102, (uint64_t)addr);
}

/**
* @brief Set GS base address.
* @param * addr
* @return 0 on success non - zero on failure
*/
static inline void set_gs_base(void *addr)
{
    wrmsr(0xc0000101, (uint64_t)addr);
}

/**
* @brief Set the address of the file system base.
* @param * addr
* @return 0 on success non - zero on failure
*/
static inline void set_fs_base(void *addr)
{
    wrmsr(0xc0000100, (uint64_t)addr);
}

/**
* @brief Get GS base address.
* @return The GS base address
*/
static inline void *get_kernel_gs_base(void)
{
    return (void *)rdmsr(0xc0000102);
}

/**
* @brief Get GS base address.
* @return The global SDRAM base address
*/
static inline void *get_gs_base(void)
{
    return (void *)rdmsr(0xc0000101);
}

/**
* @brief Get the address of the file system base.
* @return A pointer to the file system base
*/
static inline void *get_fs_base(void)
{
    return (void *)rdmsr(0xc0000100);
}

/**
* @brief Enable interrupts. This is a low - level function to enable interrupts.
* @return Nothing. Side effects : Enables interrupts
*/
static inline void enable_interrupts(void)
{
    asm("sti");
}

/**
* @brief Disable interrupts. This is used to turn off interrupt generation on a system that doesn't support them.
* @return Nothing. Side effects : Enables interrupts
*/
static inline void disable_interrupts(void)
{
    asm("cli");
}

/**
* @brief Toggle interrupts on / off
* @param state true to enable interrupts false to disable
* @return the previous state of
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

struct cpu_local *this_cpu(void);

#endif