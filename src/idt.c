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
#include "include/panic.h"

static ALIGN_16BIT
    idt_desc_t idt[IDT_MAX_DESCRIPTORS];

static idtr_t idtr;

static bool vectors[IDT_MAX_DESCRIPTORS];

void *isr_delta[256];

extern uint64_t isr_stub_table[];

extern void halt();

#define YIELD_VECTOR 0x30

/**
 * @brief Set the IDT attributes for a given interrupt.
 * @param vector The vector to set the descriptor for
 * @param isr The ISR to set the descriptor to
 * @param flags The flags to set the descriptor to
 * @param ist The IST to set the descriptor
 */
void idt_set_descriptor(uint8_t vector, uintptr_t isr, uint8_t flags, uint8_t ist)
{
    idt_desc_t *descriptor = &idt[vector];

    descriptor->base_low = isr & 0xFFFF;
    descriptor->base_mid = (isr >> 16) & 0xFFFF;
    descriptor->base_high = (isr >> 32) & 0xFFFFFFFF;
    descriptor->cs = GDTKernelBaseSelector;
    descriptor->ist = ist;
    descriptor->attributes = flags;
    descriptor->rsv0 = 0;
}

/**
 * @brief Initialize IDT and set IDT entries
 */
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

/**
 * @brief Allocate a new IDT vector.
 * @return The IDT vector or NULL if none
 */
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

/**
 * @brief Reload IDT. This is called at boot time to reload the IDT
 */
void idt_reload(void)
{

    idtr.base = (uintptr_t)&idt[0];
    idtr.limit = (uint16_t)sizeof(idt_desc_t) * IDT_MAX_DESCRIPTORS - 1;

    asm volatile("lidt %0" ::"m"(idtr)
                 : "memory");
}

/**
 * @brief Free IDT vector and its resources
 * @param vector IDT vector to free ( 0.. 15 )
 * @return True if success false if
 */
void idt_free_vector(uint8_t vector)
{
    idt_set_descriptor(vector, 0, 0, 0);
    vectors[vector] = false;
}

/**
 * @brief This is the handler for test messages.
 * @return Returns 0 on success
 */
static void test_handler()
{

    printf_("%s\n", "Bingo");
}

/**
 * @brief This test is for yield ISR.
 * @return Returns 0 on success
 */
static void yield_isr_test()
{

    printf_("%s\n", "BAM!");
}

extern void dyn_isr_handler(uint64_t isr);

/**
 * @brief IDT register test This is a test function for the ISR
 */
void idt_reg_test()
{

    uint8_t vector = idt_allocate_vector();

    if (vector == NULL)
    {

        printf_("%s\n", "Try Harder!");
        halt();
    }

    isr_delta[vector] = test_handler;

    printf_("%s", "ISR Delta Data: ");
    printf_("0x%llx\n", isr_delta[vector]);

    printf_("%s", "Allocated Test ISR at Vector: ");
    printf_("%i\n", vector);

    idt_set_descriptor(vector, isr_stub_table[vector], IDT_DESCRIPTOR_EXTERNAL, 001);

    // dyn_isr_handler(isr_delta[vector]);
}

/**
 * @brief Registers a yield ISR. This is called at boot time to register an ISR to be tested by the IDT
 */
void yield_register()
{

    uint8_t vector = idt_allocate_vector();

    if (vector == NULL)
    {

        panic("IDT VECTORS EXUSTED! That's on me mate sorry!");
    }

    isr_delta[vector] = yield_isr_test;

    printf_("%s", "ISR Delta Data: ");
    printf_("0x%llx\n", isr_delta[vector]);

    printf_("%s", "Allocated Test ISR at Vector: ");
    printf_("%i\n", vector);

    idt_set_descriptor(vector, isr_stub_table[vector], IDT_DESCRIPTOR_EXTERNAL, 001);
}
