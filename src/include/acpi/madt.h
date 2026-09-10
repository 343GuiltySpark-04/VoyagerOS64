/**
 * @file madt.h
 * @brief ACPI MADT record structures and parsed interrupt-topology lists.
 * @ingroup interrupts
 */
#ifndef _ACPI__MADT_H
#define _ACPI__MADT_H

#include "../global_defs.h"
#include "../lib/vector.h"
#include <stddef.h>
#include <stdint.h>

/** @brief Common two-byte header shared by variable-length MADT records. */
struct madt_header
{
    uint8_t id;
    uint8_t length;
} PACKED;

/** @brief MADT Processor Local APIC record. */
struct madt_lapic
{
    struct madt_header;
    uint8_t  processor_id;
    uint8_t  apic_id;
    uint32_t flags;
} PACKED;

/** @brief MADT I/O APIC record. */
struct madt_io_apic
{
    struct madt_header;
    uint8_t  apic_id;
    uint8_t  reserved;
    uint32_t address;
    uint32_t gsib;
} PACKED;

/** @brief MADT Interrupt Source Override record. */
struct madt_iso
{
    struct madt_header;
    uint8_t  bus_source;
    uint8_t  irq_source;
    uint32_t gsi;
    uint16_t flags;
} PACKED;

/** @brief MADT local-APIC NMI record. */
struct madt_nmi
{
    struct madt_header;
    uint8_t  processor;
    uint16_t flags;
    uint8_t  lint;
} PACKED;

/** Parsed processor Local APIC records. */
extern VECTOR_TYPE(struct madt_lapic *) madt_lapics;
/** Parsed I/O APIC records. */
extern VECTOR_TYPE(struct madt_io_apic *) madt_io_apics;
/** Parsed interrupt-source override records. */
extern VECTOR_TYPE(struct madt_iso *) madt_isos;
/** Parsed local NMI records. */
extern VECTOR_TYPE(struct madt_nmi *) madt_nmis;

/**
 * @brief Locate and parse the ACPI MADT into Voyager's topology vectors.
 *
 * 0.0.5 discovers this information but still qualifies the legacy PIC path as
 * the primary interrupt route; full APIC/SMP use is later work.
 */
void madt_init(void);

#endif
