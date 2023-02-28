#include "include/cpuUtils.h"
#include "include/printf.h"
#include "include/registers.h"
#include "include/global_defs.h"
#include "include/limine.h"
#include "include/memUtils.h"
#include "include/paging/frameallocator.h"
#include "include/paging/paging.h"
#include "include/proc.h"
#include "include/sched.h"
#include "include/kernel.h"
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
extern int cpuid_check_vmx();
extern int cpuid_check_htt();
extern int cpuid_check_fpu();
extern int cpuid_check_msr();
extern int cpuid_check_oxsave();
extern int cpuid_check_avx();
extern int cpuid_check_fxsr();
extern int vendor_str1();
extern int vendor_str2();
extern int vendor_str3();
extern void halt();

char CPU_vendor[13];

bool first_run = true;

/**
 * @brief Print a message to the standard output. This is called from cpuid_read ()
 */
void cpuid_readout()
{

    printf_("%s\n", "-----------------------------");
    printf_("%s\n", "|       CPUID Readout       |");
    printf_("%s\n", "-----------------------------");

    printf_("%s\n", "-----------------------------");
    printf_("%s\n", "|        Brand & Vendor     |");
    printf_("%s\n", "-----------------------------");
    get_vendor();
    printf_("%s", "CPU Vendor: ");
    printf_("%s\n", CPU_vendor);
    printf_("%s\n", "-----------------------------");
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
    check_vmx();
    check_htt();
    check_fpu();
    check_msr();

    if (first_run == true)
    {

        first_run = false;

        if (cpuid_check_fpu() == 1)
        {

            fpu_init();
        }
        else
        {

            printf_("%s\n", "ERROR: FUCK YOU NO FLOATS!");
            halt();
        }
    }

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
 * @brief Get vendor information from CPUID
 * @return 0 on success - 1 on
 */
void get_vendor()
{

    uint32_t ebx, edx, ecx, eax;

    __get_cpuid(0, &eax, &ebx, &ecx, &edx);
    for (size_t i = 0; i < 4; i++)
    {
        const size_t shiftor = i * 8;
        CPU_vendor[0 + i] = (ebx >> shiftor) & 0xFF;
        CPU_vendor[4 + i] = (edx >> shiftor) & 0xFF;
        CPU_vendor[8 + i] = (ecx >> shiftor) & 0xFF;
    }
    CPU_vendor[12] = 0;
}

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
    // writeCR0(readCRO() | 0 >> 2);

    printf_("%s\n", "INFO: x87 FPU now online");
}

/**
 * @brief Print message to indicate SSE extensions are unavailable.
 * @return non - zero for success zero for
 */
void no_sse()
{

    printf_("%s\n", "SSE Extensions Unavailable.");
    printf_("%s\n", "Floating Point Math will be offline.");
}

/**
 * @brief Display message to indicate XSAVE extensions are unavailable.
 * @return non - zero if message was displayed
 */
void no_xsave()
{

    printf_("%s\n", "XSAVE Extensions Unavailable.");
    printf_("%s\n", "Floating Point Math will be offline.");
}

/**
 * @brief Check SSE extensions and print results if they are available.
 * @return 1 if SSE extensions are available 0
 */
void check_sse()
{

    int found = cpuid_check_sse();

    if (found == 1)
    {

        printf_("%s\n", "SSE Extensions Available.");

        printf_("%s\n", "Enabling....");

        printf_("%s\n", "SSE Extensions Online!");
    }
}

/**
 * @brief Check if XSAVE is available
 * @return 1 if XSAVE is
 */
void check_xsave()
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

/**
 * @brief Check if FPU is enabled or
 */
void check_fpu()
{

    if (cpuid_check_fpu() == 1)
    {

        printf_("%s\n", "x87 FPU: Yes");
    }
    else
    {

        printf_("%s\n", "x87 FPU: No");
    }
}

/**
 * @brief Check MSR on / off
 * @return true if MSR is
 */
void check_msr()
{

    if (cpuid_check_msr() == 1)
    {

        printf_("%s\n", "MSR: Yes");
    }
    else
    {

        printf_("%s\n", "MSR: No");
    }
}

/**
 * @brief Check VMX is enabled or
 */
void check_vmx()
{

    if (cpuid_check_vmx() == 1)
    {

        printf_("%s\n", "VMX (HW Virtualization): Yes");
    }
    else
    {

        printf_("%s\n", "VMX (HW Virtualization): No");
    }
}

/**
 * @brief Check if HTT is enabled
 * @return true if HTT is
 */
void check_htt()
{

    if (cpuid_check_htt() == 1)
    {

        printf_("%s\n", "HTT: Yes");
    }
    else
    {

        printf_("%s\n", "HTT: No");
    }
}

/**
 * @brief Check if CPUID is available
 * @return Yes if available No
 */
void check_pcid()
{

    if (cpuid_check_pcid() == 1)
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
 * @brief Check if CPUID has ACPI installed
 * @return Yes if installed No
 */
void check_acpi()
{

    int found = cpuid_check_acpi();

    if (found == 1)
    {

        printf_("%s\n", "ACPI: Yes");
    }
    else
    {

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
 * @brief Check TM and print yes / no.
 * @return Yes if TM is set
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