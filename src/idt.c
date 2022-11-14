#include <stdint.h>
#include "include/string.h"
#include <stdbool.h>
#include "include/global_defs.h"
#include "include/idt.h"
#include "include/printf.h"
#include "include/gdt.h"
#include "include/registers.h"
#include "include/interrupts.h"
#include "include/lock.h"

static ALIGN_16BIT
    idt_desc_t idt[IDT_MAX_DESCRIPTORS];

static idtr_t idtr;

static bool vectors[IDT_MAX_DESCRIPTORS];

extern uint64_t isr_stub_table[];

extern void halt();

void idt_set_descriptor(uint8_t vector, uintptr_t isr, uint8_t flags, uint8_t ist)
{
    idt_desc_t *descriptor = &idt[vector];

    descriptor->base_low = isr & 0xFFFF;
    descriptor->cs = GDTKernelBaseSelector;
    descriptor->ist = ist;
    descriptor->attributes = flags;
    descriptor->base_mid = (isr >> 16) & 0xFFFF;
    descriptor->base_high = (isr >> 32) & 0xFFFFFFFF;
    descriptor->rsv0 = 0;
}

void idt_init()
{
    idtr.base = (uintptr_t)&idt[0];
    idtr.limit = (uint16_t)sizeof(idt_desc_t) * IDT_MAX_DESCRIPTORS - 1;

    for (uint8_t vector = 0; vector < IDT_CPU_EXCEPTION_COUNT + IDT_HDW_INTERRUPT_COUNT; vector++)
    {

        if (vector >= 32)
        {

            idt_set_descriptor(vector, isr_stub_table[vector], IDT_DESCRIPTOR_EXTERNAL, 001);
            vectors[vector] = true;

            // printf_("%i\n", vector);
        }
        else
        {

            idt_set_descriptor(vector, isr_stub_table[vector], IDT_DESCRIPTOR_EXCEPTION, 001);
            vectors[vector] = true;
        }
    }

    __asm__ volatile("lidt %0"
                     :
                     : "m"(idtr)); // load the new IDT
    __asm__ volatile("sti");       // set the interrupt flag
}

uint8_t idt_allocate_vector()
{
    for (unsigned int i = 0; i < IDT_MAX_DESCRIPTORS; i++)
    {
        if (!vectors[i])
        {
            vectors[i] = true;
            return (uint8_t)i;
        }
    }

    return NULL;
}


void idt_free_vector(uint8_t vector)
{
    idt_set_descriptor(vector, 0, 0, 0);
    vectors[vector] = false;
}


void idt_reg_test(){

 uint8_t vector = idt_allocate_vector();

    if (vector == NULL){

        printf_("%s\n", "Try Harder!");
        halt();


    }

    printf_("%s", "Allocated Test ISR at Vector: ");
    printf_("%i\n", vector);

    idt_set_descriptor(vector, isr_stub_table[vector], IDT_DESCRIPTOR_EXTERNAL, 001);

}
