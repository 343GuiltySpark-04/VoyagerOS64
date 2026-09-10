/**
 * @file ioapic.h
 * @brief I/O APIC redirection helpers for experimental APIC routing.
 * @ingroup interrupts
 *
 * These interfaces are retained as groundwork for later APIC-based interrupt
 * routing. VoyagerOS64 0.0.5 qualifies the remapped legacy PIC path instead.
 */
#ifndef _DEV__IOAPIC_H
#define _DEV__IOAPIC_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

/**
 * @brief Program an IRQ redirection toward one Local APIC.
 * @param lapic_id Destination Local APIC ID.
 * @param vector Destination IDT vector.
 * @param irq Legacy IRQ/source number.
 * @param status Current implementation's enable/mask selector.
 */
void io_apic_set_irq_redirect(uint32_t lapic_id,
                              uint8_t  vector,
                              uint8_t  irq,
                              bool     status);

/**
 * @brief Program a GSI redirection toward one Local APIC.
 * @param lapic_id Destination Local APIC ID.
 * @param vector Destination IDT vector.
 * @param gsi Global System Interrupt number.
 * @param flags ACPI/MADT polarity and trigger flags associated with the source.
 * @param status Current implementation's enable/mask selector.
 */
void io_apic_set_gsi_redirect(uint32_t lapic_id,
                              uint8_t  vector,
                              uint8_t  gsi,
                              uint16_t flags,
                              bool     status);

#endif
