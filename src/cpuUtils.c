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
#include <cpuid.h>
#include <stdbool.h>

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
extern void halt();

bool has_ACPI;

/**
 * @brief Print the results of the CPUID readout
 */
void cpuid_readout()
{

    printf_("%s\n", "-----------------------------");
    printf_("%s\n", "CPUID Readout As Follows");
    printf_("%s\n", "-----------------------------");

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
 * @brief Check if SEP is enabled
 * @return true if it is
 */
void check_sep()
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

        writeCR0(readCRO() | 1 << 2);

        writeCR0(readCRO() | 1 << 1);

        writeCR4(readCR4() | 1 << 9);

        writeCR4(readCR4() | 1 << 10);

        printf_("%s\n", "SSE Extensions Online!");
    }
}

/**
 * @brief Check if XSAVE is available and print a message if not.
 * @return 1 if XSAVE is
 */
void check_xsave()
{

    int found = cpuid_check_xsave();

    if (found == 1)
    {

        printf_("%s\n", "XSAVE Extensions Available.");

        printf_("%s\n", "Enabling....");

        writeCR4(readCR4() | 1 << 18);

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