#include <stdint.h>
#include "include/gdt.hpp"

extern void breakpoint();

extern "C" volatile void init_kernel()
{
    LoadGDT(&bootstrapGDT, &bootstrapTSS, bootstrapTssStack, bootstrapist1Stack, bootstrapist2Stack, sizeof(bootstrapTssStack), &bootstrapGDTR);
    // breakpoint();
}