#include "include/cpu.h"
#include "include/gdt.h"
#include "include/panic.h"
#include "include/registers.h"
#include <stdbool.h>
#include <stddef.h>

bool sysenter;

size_t fpu_bank_size;
void (*fpu_save)(void *ctx) = NULL;
void (*fpu_rest)(void *ctx) = NULL;

struct cpu_local *this_cpu(void)
{
    if (interrupt_state())
        panic("Calling this_cpu() With Interrupts on is Forbidden!");

    return NULL;
}
