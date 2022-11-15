#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include "include/cpu.h"
#include "include/registers.h"
#include "include/gdt.h"
#include "include/tss.h"
#include "include/idt.h"
#include "include/printf.h"
#include "include/liballoc.h"
#include "include/paging/frameallocator.h"
#include "include/paging/paging.h"
#include "include/global_defs.h"
#include "include/string.h"
#include "include/cpuUtils.h"
#include "include/limine.h"
#include "include/apic/lapic.h"

bool sysenter;
extern void halt();

size_t fpu_bank_size;
void (*fpu_save)(void *ctx) = NULL;
void (*fpu_rest)(void *ctx) = NULL;

#define CPU_STACK_SIZE 0x10000

static size_t cpus_started_i = 0;

static volatile struct limine_smp_request smp_request = {

    .id = LIMINE_SMP_REQUEST,
    .revision = 0

};

struct cpu_local *this_cpu(void)
{

    if (interrupt_state())
    {

        printf_("%s\n", "!!!!!!!!!!!!!!!!!!!!KERNEL PANIC!!!!!!!!!!!!!!!!!!!!!");
        printf_("%s\n", " Calling this_cpu() With Interrupts on is Forbidden!");
        printf_("%s\n", "!!!!!!!!!!!!!!!!!!!!KERNEL PANIC!!!!!!!!!!!!!!!!!!!!!");
        halt();
    }

    return NULL;
}