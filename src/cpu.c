#include "include/cpu.h"
#include "include/apic/lapic.h"
#include "include/cpuUtils.h"
#include "include/gdt.h"
#include "include/global_defs.h"
#include "include/idt.h"
#include "include/liballoc.h"
#include "include/limine.h"
#include "include/paging/frameallocator.h"
#include "include/paging/paging.h"
#include "include/panic.h"
#include "include/printf.h"
#include "include/registers.h"
#include "include/string.h"
#include "include/tss.h"
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

bool        sysenter;
extern void halt();

size_t fpu_bank_size;
void (*fpu_save)(void *ctx) = NULL;
void (*fpu_rest)(void *ctx) = NULL;

#define CPU_STACK_SIZE 0x10000

static size_t cpus_started_i = 0;

extern volatile struct limine_smp_request smp_request;

struct cpu_local *this_cpu(void)
{
    if (interrupt_state())
    {
        panic("Calling this_cpu() With Interrupts on is Forbidden!");
    }

    return NULL;
}