#include "include/cpuUtils.h"
#include "include/global_defs.h"
#include "include/kernel.h"
#include "include/limine.h"
#include "include/memUtils.h"
#include "include/paging/frameallocator.h"
#include "include/paging/paging.h"
#include "include/printf.h"
#include "include/proc.h"
#include "include/registers.h"
#include "include/sched.h"
#include "include/cpu.h"
#include "include/liballoc.h"
#include <cpuid.h>
#include <stdbool.h>
#include <stddef.h>

extern int cpuid_check_sse();
extern int cpuid_check_xsave();
extern int cpuid_check_pcid();
extern int cpuid_check_pae();
extern int cpuid_check_mce();
extern int cpuid_check_apic();
extern int cpuid_check_mca();
extern int cpuid_check_acpi();
extern int cpuid_check_ds();
extern int cpuid_check_tm();
extern int cpuid_check_sep();
extern int cpuid_check_htt();
extern int cpuid_check_rdseed();
extern int cpuid_check_rdrand();
extern int cpuid_check_fpu();
extern int cpuid_check_oxsave();
extern int cpuid_check_avx();
extern int cpuid_check_fxsr();
extern uint32_t get_apic_base_address();
extern int test_em();
extern void cfg_XCR0();
extern uint64_t rdrand_asm();
extern uint64_t rdseed_asm();
extern uint64_t read_XCR0();
extern uint64_t get_xsave_size();
extern void halt();

size_t xsave_bank ALIGN_16BIT;

bool has_ACPI;

/**
 * @brief Generate a 64 - bit pseudo - random number using RDSEED or RDRAND. This is used to generate pseudorandom numbers that are guaranteed to be in the range 0 to 2^64 - 1.
 * @return The pseudo - random number as a 64 - bit integer with at least 64 bits of entropy ( the most significant bit is zero
 */
uint64_t rand_asm()
{
    uint64_t result;

    // This function checks the cpuid_check_rdseed and cpuid_check_rdrand parameters.
    if (cpuid_check_rdseed() == 1)
    {
        result = rdseed_asm();
    }
    // This function checks if the current CPUID is a RDRAND.
    else if (cpuid_check_rdrand() == 1)
    {
        result = rdrand_asm();
    }
    else
    {
        printf_("%s\n", "FATAL: No HW RND Facilites Found!");
        halt();
    }

    return result;
}

/**
 * @brief CPUID Readout As Follows ( read CR0 CR4 ) Check all CPUIDs and print
 */
void cpuid_readout()
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

    printf_("%s", "CR0: ");
    printf_("0x%llx\n", readCRO());
    printf_("%s", "CR4: ");
    printf_("0x%llx\n", readCR4());
    printf_("%s", "APIC Base Address: ");
    printf_("0x%llx\n", get_apic_base_address());
   // halt();

    printf_("%s\n", "-----------------------------");
}

/* Example: Get CPU's model number */
/**
 * @brief Get the CPUID model.
 * @return The number of EBX
 */
int get_model(void)
{
    int ebx, unused;
    __cpuid(0, unused, ebx, unused, unused);
    return ebx;
}

/**
 * @brief \ brief Initializes x87 FPU on the system. Enables the CRO to make it work
 */
void fpu_init()
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
    // writeCR0(readCRO() | 0 >> 2);

    printf_("%s\n", "INFO: x87 FPU now online");
}

/**
 * @brief Allocate space for xsave. This is called at boot time to set up the bank of memory that will be used
 */
void alloc_xsave()
{

    xsave_bank = (size_t)get_xsave_size();

    printf_("%s", "INFO: Allocating a XSAVE Bank with a size of: ");
    printf_("0x%llx\n", (size_t)get_xsave_size());

    malloc(xsave_bank);
}

/**
 * @brief Check if FPU is available and initialize it if not. This is called at boot time to make sure we are running on CPU
 */
void check_fpu()
{

    int found = cpuid_check_fpu();

    // if found 1 print FPU Yes Yes
    if (found == 1)
    {

        printf_("%s\n", "FPU: Yes");
        fpu_init();
    }
    else
    {

        printf_("%s\n", "FPU: No");
    }
}

/**
 * @brief Checks to see if FXSR is installed on the CPU and prints Yes or No
 */
void check_fxsr()
{

    int found = cpuid_check_fxsr();

    // Prints the FXSR Yes No.
    if (found == 1)
    {

        printf_("%s\n", "FXSR: Yes");
    }
    else
    {

        printf_("%s\n", "FXSR: No");
    }
}

/**
 * @brief Prints message to indicate SSE extensions are unavailable.
 * @return void : Author Christian Schafmeister ( 1991 )
 */
void no_sse()
{

    printf_("%s\n", "SSE Extensions Unavailable.");
    printf_("%s\n", "Floating Point Math will be offline.");
}

/**
 * @brief Prints a message to indicate XSAVE extensions are unavailable.
 * @return non - zero if message was printed
 */
void no_xsave()
{

    printf_("%s\n", "XSAVE Extensions Unavailable.");
    printf_("%s\n", "Floating Point Math will be offline.");
}

/**
 * @brief \ brief Disable OXSAVE extensions and print warning to stdout. \ ingroup guile_ox
 */
void no_oxsave()
{

    printf_("%s\n", "OXSAVE Extensions Unavailable.");
    printf_("%s\n", "Floating Point Math will be offline.");
}

/**
 * @brief \ brief Print message to indicate AVX extensions are no longer available. \ return void \ par Purpose
 */
void no_avx()
{

    printf_("%s\n", "AVX Extensions Unavailable.");
    printf_("%s\n", "Floating Point Math will be offline.");
}

/**
 * @brief cpuid_check_rdseed is called from main to check if RDSEED is enabled
 */
void check_rdseed()
{

    int found = cpuid_check_rdseed();

    // Prints the RDSEED Yes No.
    if (found == 1)
    {

        printf_("%s\n", "RDSEED: Yes");
    }
    else
    {

        printf_("%s\n", "RDSEED: No");
    }
}

/**
 * @brief Check if RDRAND is available on the CPU and display if it is. This is called from cpuid_get_cpus
 */
void check_rdrand()
{

    int found = cpuid_check_rdrand();

    // Prints the RDRAND Yes No
    if (found == 1)
    {

        printf_("%s\n", "RDRAND: Yes");
    }
    else
    {

        printf_("%s\n", "RDRAND: No");
    }
}

/**
 * @brief Check if SEP is enabled
 * @return true if it is
 */
void check_sep()
{

    int found = cpuid_check_sep();

    // Prints the sysenter flag.
    if (found == 1)
    {

        sysenter = true;
        printf_("%s\n", "SEP (SYSENTER/EXIT): Yes");
    }
    else
    {

        sysenter = false;
        printf_("%s\n", "SEP (SYSENTER/EXIT): No");
        printf_("%s\n", "You Realized How Fucked You Are Without This? Halting Get A Better PC.");
        halt();
    }
}

/**
 * @brief Check SSE extensions and print results if they are available
 */
void check_sse()
{

    int found = cpuid_check_sse();

    if (found == 1)
    {

        printf_("%s\n", "SSE Extensions Available.");

        printf_("%s\n", "Enabling....");

        // writeCR0(readCRO() | 1 << 2);

        // writeCR0(readCRO() | 1 << 1);

        // writeCR4(readCR4() | 1 << 9);

        //   writeCR4(readCR4() | 1 << 10);

        printf_("%s\n", "SSE Extensions Online!");
    }
}

/**
 * @brief cpuid_check_oxsave Check to see if OXSAVE is available on the CPU and print a message if
 */
void check_oxsave()
{

    int found = cpuid_check_oxsave();

    // Prints the extension available.
    if (found == 1)
    {

        printf_("%s\n", "OXSAVE Extensions Available.");
    }
}

/**
 * @brief Check if AVX is available on the CPU and print a message if it is not. This is called from cpuid_get_avx
 */
void check_avx()
{

    int found = cpuid_check_avx();

    // Prints the available AVX Extensions Available.
    if (found = 1)
    {
        printf_("%s\n", "AVX Extensions Available.");
    }
}

/**
 * @brief Check if XSAVE is available and print a message if not.
 * @return 1 if XSAVE is
 */
void check_xsave()
{

    int found = cpuid_check_xsave();

    // Prints the XSAVE Extensions Available Enabling and XSAVE Extensions Available.
    if (found == 1)
    {

        printf_("%s\n", "XSAVE Extensions Available.");

        printf_("%s\n", "Enabling....");

        //  writeCR4(readCR4() | 1 << 18);

        printf_("%s\n", "XSAVE Extensions Online!");

        printf_("%s\n", "Floating Point Math Online using SSE and XSAVE!");
    }
}

/**
 * @brief Check if PCID is available
 * @return true if PCID is
 */
void check_pcid()
{

    int found = cpuid_check_pcid();

    // Prints PCID Yes No if found 1.
    if (found == 1)
    {

        printf_("%s\n", "PCID: Yes");
    }
    else
    {

        printf_("%s\n", "PCID: No");
    }
}

/**
 * @brief Check if PAE is installed
 * @return true if installed false
 */
void check_pae()
{

    int found = cpuid_check_pae();

    if (found == 1)
    {

        printf_("%s\n", "PAE: Yes");
    }
    else
    {

        printf_("%s\n", "PAE: No");
    }
}

/**
 * @brief cpuid_check_htt Description : Checks to see if HTT is
 */
void check_htt()
{

    int found = cpuid_check_htt();

    if (found == 1)
    {

        printf_("%s\n", "HTT: Yes");
    }
    else
    {

        printf_("%s\n", "HTT: No");
    }
}

/**
 * @brief Check if MCE is enabled
 * @return true if enabled false
 */
void check_mce()
{

    int found = cpuid_check_mce();

    if (found == 1)
    {

        printf_("%s\n", "MCE: Yes");
    }
    else
    {

        printf_("%s\n", "MCE: No");
    }
}

/**
 * @brief Check if APIC is available
 * @return true if it is
 */
void check_apic()
{

    int found = cpuid_check_apic();

    if (found == 1)
    {

        printf_("%s\n", "APIC: Yes");
    }
    else
    {

        printf_("%s\n", "APIC: No");
    }
}

/**
 * @brief Check if MCA is installed
 * @return true if MCA is
 */
void check_mca()
{

    int found = cpuid_check_mca();

    if (found == 1)
    {

        printf("%s\n", "MCA: Yes");
    }
    else
    {

        printf_("%s\n", "MCA: No");
    }
}

/**
 * @brief Check if CPUID has ACPI.
 * @return true if it has ACPI false
 */
void check_acpi()
{

    int found = cpuid_check_acpi();

    if (found == 1)
    {

        has_ACPI = true;

        printf_("%s\n", "ACPI: Yes");
    }
    else
    {

        has_ACPI = false;

        printf_("%s\n", "ACPI: No");
    }
}

/**
 * @brief Check if cpuid_check_ds is available
 * @return true if cpuid_check_ds is
 */
void check_ds()
{

    int found = cpuid_check_ds();

    if (found == 1)
    {

        printf_("%s\n", "DS: Yes");
    }
    else
    {

        printf_("%s\n", "DS: No");
    }
}

/**
 * @brief Check TM is installed on the CPU
 * @return true if TM is installed
 */
void check_tm()
{

    int found = cpuid_check_tm();

    if (found == 1)
    {

        printf_("%s\n", "TM: Yes");
    }
    else
    {

        printf_("%s\n", "TM: No");
    }
}