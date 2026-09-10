/**
 * @file cpuUtils.h
 * @brief CPUID feature discovery and early CPU capability helpers.
 * @ingroup boot
 */
#pragma once
#ifndef _CPU_UTILS_H
#define _CPU_UTILS_H

#include "registers.h"
#include "tss.h"
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

/* Vendor strings from CPUs. */
#define CPUID_VENDOR_OLDAMD \
    "AMDisbetter!" /* Early engineering samples of AMD K5 processor */
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
#define CPUID_VENDOR_PARALLELS_ALT \
    " lrpepyh vr "
#define CPUID_VENDOR_BHYVE "bhyve bhyve "
#define CPUID_VENDOR_QNX " QNXQVMBSQG "

/** CPUID-derived ACPI capability state. */
extern bool has_ACPI;
/** Historical XSAVE-state allocation bookkeeping. */
extern size_t xsave_bank;

/** @brief Obtain a hardware-generated random value through the assembly helper. */
uint64_t rand_asm(void);
/** @brief Return the detected processor model identifier used by current code. */
int get_model(void);
/** @brief Print the verbose boot-time CPUID/capability report. */
void cpuid_readout(void);
/** @brief Allocate/configure storage used by the current XSAVE experiment. */
void alloc_xsave(void);
/** @brief Initialize the current floating-point/SIMD setup path. */
void fpu_init(void);

/** Individual feature-probe helpers used by cpuid_readout(). */
void check_avx(void);
void check_oxsave(void);
void check_fxsr(void);
void check_htt(void);
void check_sep(void);
void check_sse(void);
void check_xsave(void);
void check_pcid(void);
void check_pae(void);
void check_mce(void);
void check_apic(void);
void check_mca(void);
void check_acpi(void);
void check_ds(void);
void check_tm(void);
void check_rdseed(void);
void check_rdrand(void);
void check_fpu(void);
void check_tsc(void);

/**
 * @warning The CPUID readout and the scheduler's floating-point context model
 * are separate concerns. VoyagerOS64 0.0.5 does not provide qualified per-task
 * XSAVE/XRSTOR state, and the current feature messages should not be interpreted
 * as proof that floating-point state is safely preserved across task switches.
 */

#endif
