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
extern void halt();

void cpuid_readout()
{

    printf_("%s\n", "-----------------------------");
    printf_("%s\n", "CPUID Readout As Follows");
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

    printf_("%s\n", "-----------------------------");
}

/* Example: Get CPU's model number */
int get_model(void)
{
    int ebx, unused;
    __cpuid(0, unused, ebx, unused, unused);
    return ebx;
}

void no_sse()
{

    printf_("%s\n", "SSE Extensions Unavailable.");
    printf_("%s\n", "Floating Point Math will be offline.");
}

void no_xsave()
{

    printf_("%s\n", "XSAVE Extensions Unavailable.");
    printf_("%s\n", "Floating Point Math will be offline.");
}

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