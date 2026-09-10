/**
 * @file paging.h
 * @brief Generic x86-64 page-table helpers and address-space constants.
 * @ingroup paging
 */
#pragma once
#ifndef _PAGING_H_
#define _PAGING_H_
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
#define PAGING_EXPORT extern "C"
#else
#define PAGING_EXPORT
#endif

/** Base of the Limine HHDM used by the current VoyagerOS64 configuration. */
#define HIGHER_HALF_MEMORY_OFFSET 0xFFFF800000000000
/** Link-time higher-half virtual base of the kernel image. */
#define HIGHER_HALF_KERNEL_MEMORY_OFFSET 0xFFFFFFFF80000000

/** Bits treated as non-address metadata in ordinary 4 KiB page entries. */
#define PAGE_FLAG_MASK (0xFFF | (1ull << 63))
/** Mask selecting the physical-address field of an ordinary page entry. */
#define PAGE_ADDRESS_MASK (~(PAGE_FLAG_MASK))

/**
 * @brief Convert a physical address to its HHDM virtual address.
 * @param physicalAddress Physical address to translate.
 * @return Direct-map virtual address corresponding to @p physicalAddress.
 *
 * @note This is arithmetic for the configured HHDM, not a page-table walk.
 */
static inline uint64_t
TranslateToHighHalfMemoryAddress(uint64_t physicalAddress)
{
    return physicalAddress + HIGHER_HALF_MEMORY_OFFSET;
}

/**
 * @brief Convert an HHDM virtual address back to a physical address.
 * @param virtualAddress Address in the HHDM region.
 * @return Physical address corresponding to @p virtualAddress.
 *
 * @warning This helper assumes the argument belongs to the HHDM. It is not a
 * general-purpose virtual-to-physical translator for arbitrary kernel VAs.
 */
inline uint64_t TranslateToPhysicalMemoryAddress(uint64_t virtualAddress)
{
    return virtualAddress - HIGHER_HALF_MEMORY_OFFSET;
}

/**
 * @brief Convert a higher-half kernel-image VA to its link-relative physical
 *        offset.
 * @param virtualAddress Address in the higher-half kernel-image region.
 * @return Address obtained by subtracting the kernel higher-half base.
 *
 * @note Runtime kernel physical placement is provided separately by Limine;
 * this helper performs only the arithmetic encoded here.
 */
inline uint64_t TranslateToKernelPhysicalMemoryAddress(uint64_t virtualAddress)
{
    return virtualAddress - HIGHER_HALF_KERNEL_MEMORY_OFFSET;
}

/**
 * @brief Add the kernel higher-half base to an address/offset.
 * @param virtualAddress Address/offset to translate.
 * @return Value in the kernel higher-half virtual region.
 */
inline uint64_t TranslateToKernelMemoryAddress(uint64_t virtualAddress)
{
    return virtualAddress + HIGHER_HALF_KERNEL_MEMORY_OFFSET;
}

/**
 * @brief Test whether an address value lies at or above the HHDM base.
 * @param physicalAddress Address value to check.
 * @return true when the value is in the configured higher-half range.
 */
inline bool IsHigherHalf(uint64_t physicalAddress)
{
    return physicalAddress >= HIGHER_HALF_MEMORY_OFFSET;
}

/**
 * @brief Test whether an address value lies at or above the kernel-image base.
 * @param physicalAddress Address value to check.
 * @return true when the value is in the configured kernel higher-half range.
 */
inline bool IsKernelHigherHalf(uint64_t physicalAddress)
{
    return physicalAddress >= HIGHER_HALF_KERNEL_MEMORY_OFFSET;
}

/** @brief x86-64 page-entry flag values used by VoyagerOS64. */
enum PagingFlag
{
    PAGING_FLAG_PRESENT         = (1ull << 0),
    PAGING_FLAG_WRITABLE        = (1ull << 1),
    PAGING_FLAG_USER_ACCESSIBLE = (1ull << 2),
    PAGING_FLAG_PAT0            = (1ull << 3),
    PAGING_FLAG_PAT1            = (1ull << 4),
    PAGING_FLAG_PAT2            = (1ull << 7),
    PAGING_FLAG_WRITE_COMBINE   = PAGING_FLAG_PAT0 | PAGING_FLAG_PAT1,
    PAGING_FLAG_LARGER_PAGES    = (1ull << 7),
    PAGING_FLAG_NO_EXECUTE      = (1ull << 63),
};

/** @brief Four-level x86-64 page-table indices for a virtual address. */
struct PageTableOffset
{
    uint64_t p4Offset;
    uint64_t pdpOffset;
    uint64_t pdOffset;
    uint64_t ptOffset;
};

/**
 * @brief One 4 KiB x86-64 page table containing 512 entries.
 */
struct __attribute__((aligned(0x1000))) PageTable
{
    uint64_t entries[512];
};

/**
 * @brief Map one 4 KiB virtual page to one physical page.
 * @param p4 Physical address of the root PML4 used by this paging layer.
 * @param virtualMemory Virtual page address to map.
 * @param physicalMemory Physical page address to install in the leaf entry.
 * @param flags Leaf-page flags in addition to the present bit.
 *
 * Missing intermediate tables are allocated through PagingGetFreeFrame().
 */
PAGING_EXPORT void PagingMapMemory(struct PageTable *p4,
                                   void             *virtualMemory,
                                   void             *physicalMemory,
                                   uint64_t          flags);

/**
 * @brief Clear the leaf mapping for one virtual page.
 * @param p4 Physical address of the root PML4.
 * @param virtualMemory Virtual page whose leaf entry should be cleared.
 *
 * This does not free intermediate page tables or the previously mapped physical
 * frame.
 */
PAGING_EXPORT void PagingUnmapMemory(struct PageTable *p4, void *virtualMemory);

/**
 * @brief Identity-map one page so its virtual and physical addresses match.
 * @param p4 Physical address of the root PML4.
 * @param virtualMemory Address used as both virtual and physical page address.
 * @param flags Leaf-page flags.
 */
PAGING_EXPORT void
PagingIdentityMap(struct PageTable *p4, void *virtualMemory, uint64_t flags);

/**
 * @brief Resolve the physical page backing a mapped virtual page.
 * @param p4 Physical address of the root PML4.
 * @param virtualMemory Virtual address to inspect.
 * @return Physical page address encoded by the leaf entry, or NULL when an
 *         intermediate level is not present.
 */
PAGING_EXPORT void *PagingPhysicalMemory(struct PageTable *p4,
                                         void             *virtualMemory);

/**
 * @brief Duplicate the current paging structure into @p newTable.
 * @param p4 Physical address of the source root PML4.
 * @param newTable Destination root table accessible to the kernel.
 *
 * The current implementation recursively duplicates the lower half and shares
 * the upper-half root entries. VoyagerOS64 0.0.5 does not yet expose this as a
 * complete per-process address-space or fork facility.
 */
PAGING_EXPORT void PagingDuplicate(struct PageTable *p4,
                                   struct PageTable *newTable);

/**
 * @brief Install @p p4 as the active page-table root.
 * @param p4 Root table expected by the implementation.
 *
 * Prefer paging_bootstrap() for the initial 0.0.5 kernel CR3 handoff; this
 * function belongs to the generic paging helper layer.
 */
PAGING_EXPORT void PagingSetActivePageTable(struct PageTable *p4);

#endif