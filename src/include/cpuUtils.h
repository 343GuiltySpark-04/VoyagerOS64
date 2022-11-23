#pragma once
#ifndef _CPU_UTILS_H
#define _CPU_UTILS_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include "tss.h"

/* Vendor strings from CPUs. */
#define CPUID_VENDOR_OLDAMD "AMDisbetter!" // Early engineering samples of AMD K5 processor
#define CPUID_VENDOR_AMD "AuthenticAMD"
#define CPUID_VENDOR_INTEL "GenuineIntel"
#define CPUID_VENDOR_VIA "VIA VIA VIA "
#define CPUID_VENDOR_OLDTRANSMETA "TransmetaCPU"
#define CPUID_VENDOR_TRANSMETA "GenuineTMx86"
#define CPUID_VENDOR_CYRIX "CyrixInstead"
#define CPUID_VENDOR_CENTAUR "CentaurHauls"
#define CPUID_VENDOR_NEXGEN "NexGenDriven"
#define CPUID_VENDOR_UMC "UMC UMC UMC "
#define CPUID_VENDOR_SIS "SiS SiS SiS "
#define CPUID_VENDOR_NSC "Geode by NSC"
#define CPUID_VENDOR_RISE "RiseRiseRise"
#define CPUID_VENDOR_VORTEX "Vortex86 SoC"
#define CPUID_VENDOR_OLDAO486 "GenuineAO486"
#define CPUID_VENDOR_AO486 "MiSTer AO486"
#define CPUID_VENDOR_ZHAOXIN "  Shanghai  "
#define CPUID_VENDOR_HYGON "HygonGenuine"
#define CPUID_VENDOR_ELBRUS "E2K MACHINE "

/* Vendor strings from hypervisors. */
#define CPUID_VENDOR_QEMU "TCGTCGTCGTCG"
#define CPUID_VENDOR_KVM " KVMKVMKVM  "
#define CPUID_VENDOR_VMWARE "VMwareVMware"
#define CPUID_VENDOR_VIRTUALBOX "VBoxVBoxVBox"
#define CPUID_VENDOR_XEN "XenVMMXenVMM"
#define CPUID_VENDOR_HYPERV "Microsoft Hv"
#define CPUID_VENDOR_PARALLELS " prl hyperv "
#define CPUID_VENDOR_PARALLELS_ALT " lrpepyh vr " // Sometimes Parallels incorrectly encodes "prl hyperv" as "lrpepyh vr" due to an endianness mismatch.
#define CPUID_VENDOR_BHYVE "bhyve bhyve "
#define CPUID_VENDOR_QNX " QNXQVMBSQG "

extern bool sysenter;

extern char CPU_vendor[];

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
    struct TSS tss;
    struct thread *idle_thread;
    void (*timer_function)(int, struct cpu_ctx *);
};

void cpu_init(void);

extern size_t fpu_bank_size;
extern void (*fpu_save)(void *ctx);
extern void (*fpu_rest)(void *ctx);

static inline void xsave(void *ctx)
{
    asm volatile(
        "xsave (%0)"
        :
        : "r"(ctx), "a"(0xffffffff), "d"(0xffffffff)
        : "memory");
}

static inline void xrstor(void *ctx)
{
    asm volatile(
        "xrstor (%0)"
        :
        : "r"(ctx), "a"(0xffffffff), "d"(0xffffffff)
        : "memory");
}

static inline void fxsave(void *ctx)
{
    asm volatile(
        "fxsave (%0)"
        :
        : "r"(ctx)
        : "memory");
}

static inline void fxrstor(void *ctx)
{
    asm volatile(
        "fxrstor (%0)"
        :
        : "r"(ctx)
        : "memory");
}

static inline bool interrupt_state(void)
{
    uint64_t flags;
    asm volatile("pushfq; pop %0"
                 : "=rm"(flags));
    return flags & (1 << 9);
}

static inline uint64_t rdmsr(uint32_t msr)
{
    uint32_t edx = 0, eax = 0;
    asm volatile(
        "rdmsr\n\t"
        : "=a"(eax), "=d"(edx)
        : "c"(msr)
        : "memory");
    return ((uint64_t)edx << 32) | eax;
}

static inline uint64_t wrmsr(uint32_t msr, uint64_t val)
{
    uint32_t eax = (uint32_t)val;
    uint32_t edx = (uint32_t)(val >> 32);
    asm volatile(
        "wrmsr\n\t"
        :
        : "a"(eax), "d"(edx), "c"(msr)
        : "memory");
    return ((uint64_t)edx << 32) | eax;
}

static inline void set_kernel_gs_base(void *addr)
{
    wrmsr(0xc0000102, (uint64_t)addr);
}

static inline void set_gs_base(void *addr)
{
    wrmsr(0xc0000101, (uint64_t)addr);
}

static inline void set_fs_base(void *addr)
{
    wrmsr(0xc0000100, (uint64_t)addr);
}

static inline void *get_kernel_gs_base(void)
{
    return (void *)rdmsr(0xc0000102);
}

static inline void *get_gs_base(void)
{
    return (void *)rdmsr(0xc0000101);
}

static inline void *get_fs_base(void)
{
    return (void *)rdmsr(0xc0000100);
}

struct cpu_local *this_cpu(void);

int get_model(void);
void cpuid_readout();
void detect_cpu(void);

#endif