#include "include/cpuUtils.h"
#include "include/KernelUtils.h"
#include "include/cpu.h"
#include "include/global_defs.h"
#include "include/mm/kmalloc.h"
#include "include/panic.h"
#include "include/printf.h"
#include "include/registers.h"
#include <cpuid.h>
#include <stdbool.h>
#include <stddef.h>

extern int      cpuid_check_sse(void);
extern int      cpuid_check_xsave(void);
extern int      cpuid_check_pcid(void);
extern int      cpuid_check_pae(void);
extern int      cpuid_check_mce(void);
extern int      cpuid_check_apic(void);
extern int      cpuid_check_mca(void);
extern int      cpuid_check_acpi(void);
extern int      cpuid_check_ds(void);
extern int      cpuid_check_tm(void);
extern int      cpuid_check_sep(void);
extern int      cpuid_check_htt(void);
extern int      cpuid_check_rdseed(void);
extern int      cpuid_check_rdrand(void);
extern int      cpuid_check_fpu(void);
extern int      cpuid_check_oxsave(void);
extern int      cpuid_check_avx(void);
extern int      cpuid_check_fxsr(void);
extern int      cpuid_check_tsc(void);
extern uint32_t get_apic_base_address(void);
extern int      test_em(void);
extern void     cfg_XCR0(void);
extern uint64_t rdrand_asm(void);
extern uint64_t rdseed_asm(void);
extern uint64_t read_XCR0(void);
extern uint64_t get_xsave_size(void);
extern void     halt(void);

size_t xsave_bank ALIGN_16BIT;
bool              has_ACPI;

uint64_t rand_asm(void)
{
    uint64_t result;
    uint64_t seed = cpuid_check_rdseed();
    uint64_t rand = cpuid_check_rdrand();

    if (seed == 1)
        result = rdseed_asm();
    else if (rand == 1)
        result = rdrand_asm();
    else
        panic("NO HARDWARE RNG CAPABILITES DETECTED!");

    return result;
}

void cpuid_readout(void)
{
    printf_("%s\n", "-----------------------------");
    printf_("%s\n", "CPUID Readout As Follows");
    printf_("%s\n", "-----------------------------");

    check_avx();
    check_oxsave();
    check_fxsr();
    check_htt();
    check_sep();
    check_sse();
    check_xsave();
    check_pcid();
    check_pae();
    check_mce();
    check_apic();
    check_mca();
    check_acpi();
    check_ds();
    check_tm();
    check_rdseed();
    check_rdrand();
    check_fpu();
    check_tsc();

    printf_("%s", "CR0: ");
    printf_("0x%llx\n", readCRO());
    printf_("%s", "CR4: ");
    printf_("0x%llx\n", readCR4());
    printf_("%s", "APIC Base Address: ");
    printf_("0x%llx\n", get_apic_base_address());
    printf_("%s\n", "-----------------------------");
}

int get_model(void)
{
    int ebx, unused;
    __cpuid(0, unused, ebx, unused, unused);
    return ebx;
}

void fpu_init(void)
{
    printf_("%s\n", "INFO: Enabling the x87 FPU");

    writeCR0(readCRO() | 1 << 1);
    writeCR0(readCRO() | 1 << 5);
    writeCR4(readCR4() | 1 << 9);
    writeCR4(readCR4() | 1 << 10);
    writeCR4(readCR4() | 1 << 18);
    cfg_XCR0();
    printf_("%s", "XCR0: ");
    printf_("0x%llx\n", read_XCR0());
    printf_("%s", "XSTORE Area size: ");
    printf_("0x%llx\n", get_xsave_size());
    printf_("%s\n", "INFO: x87 FPU now online");
}

void alloc_xsave(void)
{
    xsave_bank = (size_t) get_xsave_size();
    printf_("%s", "INFO: Allocating a XSAVE Bank with a size of: ");
    printf_("0x%llx\n", xsave_bank);

    /* The historical XSAVE code never retained this pointer. Keep the old
     * behavior off liballoc while the FPU subsystem remains parked. */
    (void) kmalloc(xsave_bank);
}

void check_fpu(void)
{
    int found = cpuid_check_fpu();
    if (found == 1)
    {
        printf_("%s\n", "FPU: Yes");
        if (k_mode.fpu_allowed == 1)
            fpu_init();
    }
    else
    {
        printf_("%s\n", "FPU: No");
    }
}

void check_fxsr(void)
{
    int found = cpuid_check_fxsr();
    printf_("%s\n", found == 1 ? "FXSR: Yes" : "FXSR: No");
}

void no_sse(void)
{
    printf_("%s\n", "SSE Extensions Unavailable.");
    printf_("%s\n", "Floating Point Math will be offline.");
}

void no_xsave(void)
{
    printf_("%s\n", "XSAVE Extensions Unavailable.");
    printf_("%s\n", "Floating Point Math will be offline.");
}

void no_oxsave(void)
{
    printf_("%s\n", "OXSAVE Extensions Unavailable.");
    printf_("%s\n", "Floating Point Math will be offline.");
}

void no_avx(void)
{
    printf_("%s\n", "AVX Extensions Unavailable.");
    printf_("%s\n", "Floating Point Math will be offline.");
}

void check_tsc(void)
{
    int found = cpuid_check_tsc();
    printf_("%s\n", found == 1 ? "TSC: YES" : "TSC: NO");
}

void check_rdseed(void)
{
    int found = cpuid_check_rdseed();
    printf_("%s\n", found == 1 ? "RDSEED: Yes" : "RDSEED: No");
}

void check_rdrand(void)
{
    int found = cpuid_check_rdrand();
    printf_("%s\n", found == 1 ? "RDRAND: Yes" : "RDRAND: No");
}

void check_sep(void)
{
    int found = cpuid_check_sep();
    if (found == 1)
    {
        sysenter = true;
        printf_("%s\n", "SEP (SYSENTER/EXIT): Yes");
    }
    else
    {
        sysenter = false;
        printf_("%s\n", "SEP (SYSENTER/EXIT): No");
        printf_("%s\n",
                "You Realized How Fucked You Are Without This? Halting Get A "
                "Better PC.");
        halt();
    }
}

void check_sse(void)
{
    int found = cpuid_check_sse();
    if (found == 1)
    {
        printf_("%s\n", "SSE Extensions Available.");
        printf_("%s\n", "Enabling....");
        printf_("%s\n", "SSE Extensions Online!");
    }
}

void check_oxsave(void)
{
    int found = cpuid_check_oxsave();
    if (found == 1)
        printf_("%s\n", "OXSAVE Extensions Available.");
}

void check_avx(void)
{
    int found = cpuid_check_avx();
    if (found == 1)
        printf_("%s\n", "AVX Extensions Available.");
}

void check_xsave(void)
{
    int found = cpuid_check_xsave();
    if (found == 1)
    {
        printf_("%s\n", "XSAVE Extensions Available.");
        printf_("%s\n", "Enabling....");
        printf_("%s\n", "XSAVE Extensions Online!");
        printf_("%s\n", "Floating Point Math Online using SSE and XSAVE!");
    }
}

void check_pcid(void)
{
    int found = cpuid_check_pcid();
    printf_("%s\n", found == 1 ? "PCID: Yes" : "PCID: No");
}

void check_pae(void)
{
    int found = cpuid_check_pae();
    printf_("%s\n", found == 1 ? "PAE: Yes" : "PAE: No");
}

void check_htt(void)
{
    int found = cpuid_check_htt();
    printf_("%s\n", found == 1 ? "HTT: Yes" : "HTT: No");
}

void check_mce(void)
{
    int found = cpuid_check_mce();
    printf_("%s\n", found == 1 ? "MCE: Yes" : "MCE: No");
}

void check_apic(void)
{
    int found = cpuid_check_apic();
    printf_("%s\n", found == 1 ? "APIC: Yes" : "APIC: No");
}

void check_mca(void)
{
    int found = cpuid_check_mca();
    printf_("%s\n", found == 1 ? "MCA: Yes" : "MCA: No");
}

void check_acpi(void)
{
    int found = cpuid_check_acpi();
    has_ACPI  = found == 1;
    printf_("%s\n", has_ACPI ? "ACPI: Yes" : "ACPI: No");
}

void check_ds(void)
{
    int found = cpuid_check_ds();
    printf_("%s\n", found == 1 ? "DS: Yes" : "DS: No");
}

void check_tm(void)
{
    int found = cpuid_check_tm();
    printf_("%s\n", found == 1 ? "TM: Yes" : "TM: No");
}
