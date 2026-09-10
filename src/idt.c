#include "include/idt.h"
#include "include/gdt.h"
#include "include/global_defs.h"
#include "include/interrupts.h"
#include "include/lock.h"
#include "include/panic.h"
#include "include/printf.h"
#include "include/registers.h"
#include "include/string.h"
#include <stdbool.h>
#include <stdint.h>

static ALIGN_16BIT idt_desc_t idt[IDT_MAX_DESCRIPTORS];
static idtr_t                 idtr;
static bool                   vectors[IDT_MAX_DESCRIPTORS];

void           *isr_delta[256];
extern uint64_t isr_stub_table[];
extern void     halt(void);

#define YIELD_VECTOR 0x30

void idt_set_descriptor(uint8_t   vector,
                        uintptr_t isr,
                        uint8_t   flags,
                        uint8_t   ist)
{
    idt_desc_t *descriptor = &idt[vector];

    descriptor->base_low   = isr & 0xFFFF;
    descriptor->base_mid   = (isr >> 16) & 0xFFFF;
    descriptor->base_high  = (isr >> 32) & 0xFFFFFFFF;
    descriptor->cs         = GDTKernelBaseSelector;
    descriptor->ist        = ist;
    descriptor->attributes = flags;
    descriptor->rsv0       = 0;
}

void idt_init(void)
{
    idtr.base  = (uintptr_t) &idt[0];
    idtr.limit = (uint16_t) sizeof(idt_desc_t) * IDT_MAX_DESCRIPTORS - 1;

    for (uint8_t vector = 0;
         vector < IDT_CPU_EXCEPTION_COUNT + IDT_HDW_INTERRUPT_COUNT;
         vector++)
    {
        if (vector >= 32)
        {
            idt_set_descriptor(
                vector, isr_stub_table[vector], IDT_DESCRIPTOR_EXTERNAL, 001);
        }
        else
        {
            idt_set_descriptor(
                vector, isr_stub_table[vector], IDT_DESCRIPTOR_EXCEPTION, 001);
        }
        vectors[vector] = true;
    }

    __asm__ volatile("lidt %0" : : "m"(idtr));
    __asm__ volatile("sti");
}

/**
 * @brief Allocate a new IDT vector.
 * @return Allocated vector, or 0 if none remain. Vector 0 is permanently
 * reserved by the CPU exception table, so it is safe as the failure sentinel.
 */
uint8_t idt_allocate_vector(void)
{
    for (unsigned int i = 0; i < IDT_MAX_DESCRIPTORS; i++)
    {
        if (!vectors[i])
        {
            vectors[i] = true;
            return (uint8_t) i;
        }
    }

    return 0;
}

void idt_reload(void)
{
    idtr.base  = (uintptr_t) &idt[0];
    idtr.limit = (uint16_t) sizeof(idt_desc_t) * IDT_MAX_DESCRIPTORS - 1;

    asm volatile("lidt %0" ::"m"(idtr) : "memory");
}

void idt_free_vector(uint8_t vector)
{
    idt_set_descriptor(vector, 0, 0, 0);
    vectors[vector] = false;
}

static void test_handler(void)
{
    printf_("%s\n", "Bingo");
}

static void yield_isr_test(void)
{
    printf_("%s\n", "BAM!");
}

void idt_reg_test(void)
{
    uint8_t vector = idt_allocate_vector();

    if (vector == 0)
    {
        printf_("%s\n", "Try Harder!");
        halt();
    }

    isr_delta[vector] = test_handler;

    printf_("%s", "ISR Delta Data: ");
    printf_("0x%llx\n", isr_delta[vector]);
    printf_("%s", "Allocated Test ISR at Vector: ");
    printf_("%i\n", vector);

    idt_set_descriptor(
        vector, isr_stub_table[vector], IDT_DESCRIPTOR_EXTERNAL, 001);
}

void yield_register(void)
{
    uint8_t vector = idt_allocate_vector();

    if (vector == 0)
        panic("IDT VECTORS EXAUSTED!");

    isr_delta[vector] = yield_isr_test;

    printf_("%s", "ISR Delta Data: ");
    printf_("0x%llx\n", isr_delta[vector]);
    printf_("%s", "Allocated Test ISR at Vector: ");
    printf_("%i\n", vector);

    idt_set_descriptor(
        vector, isr_stub_table[vector], IDT_DESCRIPTOR_EXTERNAL, 001);
}
