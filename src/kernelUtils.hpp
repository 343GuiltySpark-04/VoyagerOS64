#pragma once

#include "include/gdt.hpp"
#include <stdint.h>

extern "C" void init_kernel()
{
    LoadGDT(&bootstrapGDT, &bootstrapTSS, bootstrapTssStack, bootstrapist1Stack, bootstrapist2Stack, sizeof(bootstrapTssStack), &bootstrapGDTR);
}