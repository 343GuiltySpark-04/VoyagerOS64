/**
 * @file acpi.h
 * @brief ACPI table discovery and SDT access interface.
 * @ingroup boot
 */
#ifndef _ACPI__ACPI_H
#define _ACPI__ACPI_H

#include <stddef.h>
#include <stdint.h>

/**
 * @brief Common ACPI System Description Table header.
 *
 * The fixed header is shared by ACPI tables located through acpi_find_sdt().
 * Table-specific structures begin after/around this header as defined by ACPI.
 */
struct sdt
{
    char     signature[4];
    uint32_t length;
    uint8_t  revision;
    uint8_t  checksum;
    char     oem_id[6];
    char     oem_table_id[6];
    uint32_t oem_revision;
    uint32_t creator_id;
    uint32_t creator_revision;
};

/**
 * @brief Initialize Voyager's ACPI table-discovery state.
 *
 * Called during kernel bring-up when CPUID/platform probing reports ACPI support.
 * Mandatory-table failures are routed through the kernel panic/error path.
 */
void acpi_init(void);

/**
 * @brief Locate an ACPI SDT by four-byte signature and occurrence index.
 * @param signature Four-character ACPI table signature; no NUL terminator is
 *                  required by the API contract.
 * @param index Zero-based occurrence index when more than one matching table
 *              exists.
 * @return Kernel-accessible pointer to the matching table, or NULL if absent.
 */
void *acpi_find_sdt(const char signature[static 4], size_t index);

#endif
