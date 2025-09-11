#ifndef _PAGING_H_
#define _PAGING_H_
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
#define PAGING_EXPORT extern "C"
#else
#define PAGING_EXPORT
#endif

#define HIGHER_HALF_MEMORY_OFFSET 0xFFFF800000000000
#define HIGHER_HALF_KERNEL_MEMORY_OFFSET 0xFFFFFFFF80000000

#define PAGE_FLAG_MASK (0xFFF | (1ull << 63))
#define PAGE_ADDRESS_MASK (~(PAGE_FLAG_MASK))

/**
* @brief Translates a physical address to a high half memory address.
* @param physicalAddress The physical address to translate.
* @return The translated high half memory address
*/
inline uint64_t TranslateToHighHalfMemoryAddress(uint64_t physicalAddress)
{
    return physicalAddress + HIGHER_HALF_MEMORY_OFFSET;
}

/**
* @brief Translates a virtual address to a physical memory address.
* @param virtualAddress The virtual address to translate.
* @return The physical address that corresponds to the given virtual address
*/
inline uint64_t TranslateToPhysicalMemoryAddress(uint64_t virtualAddress)
{
    return virtualAddress - HIGHER_HALF_MEMORY_OFFSET;
}

/**
* @brief Translates a virtual address to a kernel physical memory address.
* @param virtualAddress The virtual address to translate.
* @return The translated kernel physical memory address
*/
inline uint64_t TranslateToKernelPhysicalMemoryAddress(uint64_t virtualAddress)
{
    return virtualAddress - HIGHER_HALF_KERNEL_MEMORY_OFFSET;
}

/**
* @brief Translates a virtual address to kernel memory
* @param virtualAddress The virtual address to translate
* @return The translated address in kernel
*/
inline uint64_t TranslateToKernelMemoryAddress(uint64_t virtualAddress)
{
    return virtualAddress + HIGHER_HALF_KERNEL_MEMORY_OFFSET;
}

/**
* @brief Checks if the physical address is higher half of the memory.
* @param physicalAddress The physical address to check.
* @return True if the physical address is higher half
*/
inline bool IsHigherHalf(uint64_t physicalAddress)
{
    return physicalAddress >= HIGHER_HALF_MEMORY_OFFSET;
}

/**
* @brief Checks if the given physical address is higher half of kernel memory.
* @param physicalAddress The physical address to check.
* @return True if the memory belongs to HIGHER_HALF_KERNEL
*/
inline bool IsKernelHigherHalf(uint64_t physicalAddress)
{
    return physicalAddress >= HIGHER_HALF_KERNEL_MEMORY_OFFSET;
}

enum PagingFlag
{
    PAGING_FLAG_PRESENT = (1ull << 0),
    PAGING_FLAG_WRITABLE = (1ull << 1),
    PAGING_FLAG_USER_ACCESSIBLE = (1ull << 2),
    PAGING_FLAG_PAT0 = (1ull << 3),
    PAGING_FLAG_PAT1 = (1ull << 4),
    PAGING_FLAG_PAT2 = (1ull << 7),
    PAGING_FLAG_WRITE_COMBINE = PAGING_FLAG_PAT0 | PAGING_FLAG_PAT1,
    PAGING_FLAG_LARGER_PAGES = (1ull << 7),
    PAGING_FLAG_NO_EXECUTE = (1ull << 63),
};

struct PageTableOffset
{
    uint64_t p4Offset;
    uint64_t pdpOffset;
    uint64_t pdOffset;
    uint64_t ptOffset;
};

/**
* @brief Allocate and initialize a page table.
* @return A pointer to the page table
*/
struct __attribute__((aligned(0x1000))) PageTable
{
    uint64_t entries[512];
};

PAGING_EXPORT void PagingMapMemory(struct PageTable *p4, void *virtualMemory, void *physicalMemory, uint64_t flags);
PAGING_EXPORT void PagingUnmapMemory(struct PageTable *p4, void *virtualMemory);
PAGING_EXPORT void PagingIdentityMap(struct PageTable *p4, void *virtualMemory, uint64_t flags);
PAGING_EXPORT void *PagingPhysicalMemory(struct PageTable *p4, void *virtualMemory);
PAGING_EXPORT void PagingDuplicate(struct PageTable *p4, struct PageTable *newTable);
PAGING_EXPORT void PagingSetActivePageTable(struct PageTable *p4);

PAGING_EXPORT uint64_t PagingGetFreeFrame();
#endif