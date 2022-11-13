#ifndef _DEV__IOAPIC_H
#define _DEV__IOAPIC_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

void io_apic_set_irq_redirect(uint32_t lapic_id, uint8_t vector, uint8_t irq, bool status);
void io_apic_set_gsi_redirect(uint32_t lapic_id, uint8_t vector, uint8_t gsi,
                              uint16_t flags, bool status);

#endif
