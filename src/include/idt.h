/**
 * @file idt.h
 * @brief x86-64 Interrupt Descriptor Table and dynamic vector interface.
 * @ingroup interrupts
 */
#pragma once

#include "global_defs.h"
#include <stdbool.h>
#include <stdint.h>

/** Architectural maximum number of IDT entries. */
#define IDT_MAX_DESCRIPTORS 256
/** Number of architecturally reserved CPU exception vectors. */
#define IDT_CPU_EXCEPTION_COUNT 32
/** Legacy PIC hardware-interrupt vector count. */
#define IDT_HDW_INTERRUPT_COUNT 16

#define IDT_DESCRIPTOR_X16_INTERRUPT 0x06
#define IDT_DESCRIPTOR_X16_TRAP 0x07
#define IDT_DESCRIPTOR_X32_TASK 0x05
#define IDT_DESCRIPTOR_X32_INTERRUPT 0x0E
#define IDT_DESCRIPTOR_X32_TRAP 0x0F
#define IDT_DESCRIPTOR_RING1 0x40
#define IDT_DESCRIPTOR_RING2 0x20
#define IDT_DESCRIPTOR_RING3 0x60
#define IDT_DESCRIPTOR_PRESENT 0x80

/** Descriptor attributes used for exception gates. */
#define IDT_DESCRIPTOR_EXCEPTION \
    (IDT_DESCRIPTOR_X32_INTERRUPT | IDT_DESCRIPTOR_PRESENT)
/** Descriptor attributes used for external interrupt gates. */
#define IDT_DESCRIPTOR_EXTERNAL \
    (IDT_DESCRIPTOR_X32_INTERRUPT | IDT_DESCRIPTOR_PRESENT)
/** Descriptor attributes for ring-3 callable interrupt gates. */
#define IDT_DESCRIPTOR_CALL                                  \
    (IDT_DESCRIPTOR_X32_INTERRUPT | IDT_DESCRIPTOR_PRESENT | \
     IDT_DESCRIPTOR_RING3)

/** Assembly-generated ISR entry-stub table. */
extern uint64_t isr_stub_table[];

/** Dynamic ISR dispatch targets indexed by vector-specific glue. */
extern void *isr_delta[];

/** @brief Packed 16-byte x86-64 IDT gate descriptor. */
typedef struct
{
    uint16_t base_low;
    uint16_t cs;
    uint8_t  ist;
    uint8_t  attributes;
    uint16_t base_mid;
    uint32_t base_high;
    uint32_t rsv0;
} __attribute__((packed)) idt_desc_t;

/** @brief Operand format consumed by LIDT. */
typedef struct
{
    uint16_t limit;
    uint64_t base;
} __attribute__((packed)) idtr_t;

/** @brief Reload the processor IDTR from Voyager's current descriptor table. */
void idt_reload(void);

/**
 * @brief Reserve a vector from Voyager's dynamic interrupt-vector pool.
 * @return Allocated vector number.
 */
uint8_t idt_allocate_vector(void);

/**
 * @brief Release a vector previously obtained from idt_allocate_vector().
 * @param vector Vector number to release.
 */
void idt_free_vector(uint8_t vector);

/**
 * @brief Install one IDT descriptor.
 * @param vector IDT vector number.
 * @param isr Address of the interrupt entry stub/handler.
 * @param flags Gate attribute byte.
 * @param ist Interrupt Stack Table selector field.
 */
void idt_set_descriptor(uint8_t vector,
                        uintptr_t isr,
                        uint8_t flags,
                        uint8_t ist);

/** @brief Build and install the kernel's initial IDT. */
void idt_init(void);

/** @brief Boot-time self-test for dynamic ISR registration/dispatch. */
void idt_reg_test(void);

/** @brief Register the software-yield test vector used by current bring-up code. */
void yield_register(void);
