#include "include/paging/paging.h"
#include "include/paging/frameallocator.h"
#include "include/string.h"
#include "include/printf.h"
#include "include/KernelUtils.h"

/**
 * @brief Converts a virtual address to a page table offset.
 * @param * virtualAddress
 * @return The page table offset corresponding to the address
 */
struct PageTableOffset VirtualAddressToOffsets(void *virtualAddress)
{
    uint64_t address = (uint64_t)virtualAddress;

    struct PageTableOffset offset = {

        .p4Offset = (address & ((uint64_t)0x1FF << 39)) >> 39,
        .pdpOffset = (address & ((uint64_t)0x1FF << 30)) >> 30,
        .pdOffset = (address & ((uint64_t)0x1FF << 21)) >> 21,
        .ptOffset = (address & ((uint64_t)0x1FF << 12)) >> 12,
    };

    return offset;
}

/**
 * @brief Converts a page table offset to a virtual address.
 * @param offset The offset to convert.
 * @return The virtual address corresponding to the offset
 */
void *OffsetToVirtualAddress(struct PageTableOffset offset)
{
    uint64_t address = 0;

    address |= offset.p4Offset << 39;
    address |= offset.pdpOffset << 30;
    address |= offset.pdOffset << 21;
    address |= offset.ptOffset << 12;

    return (void *)address;
}

/**
 * @brief Reads and returns CR3.
 * @return Value of CR3 register
 */
uint64_t ReadCR3()
{
    uint64_t outValue = 0;

    asm("mov %%cr3, %0"
        : "=r"(outValue)
        :);

    return outValue;
    // no input
}

/**
 * @brief Write 64 - bit value to CR3
 * @param value Value to write to CR3
 * @return 0 on success non -
 */
void WriteCR3(uint64_t value)
{
    asm("mov %0, %%cr3"
        : // no output
        : "r"(value));
}

/**
 * @brief Gets or allocates an entry.
 * @param * table
 * @param offset Offset of the entry to be looked up.
 * @param flags Flags to be set on the entry.
 * @return Pointer to the entry or NULL if not found
 */
static inline struct PageTable *GetOrAllocEntry(struct PageTable *table, uint64_t offset, uint64_t flags)
{
    uint64_t address = table->entries[offset];

    if (!(address & PAGING_FLAG_PRESENT))
    {
        address = table->entries[offset] = PagingGetFreeFrame();

        if (!address)
        {
            return NULL;
        }

        table->entries[offset] |= flags | PAGING_FLAG_PRESENT;

        memset((void *)TranslateToHighHalfMemoryAddress(address), 0, 0x1000);
    }

    return (struct PageTable *)TranslateToHighHalfMemoryAddress(address & PAGE_ADDRESS_MASK);
}

/**
 * @brief Get or nullify an entry.
 * @param * table
 * @param offset The offset in the page table.
 * @return The entry or NULL if not present
 */
static inline struct PageTable *GetOrNullifyEntry(struct PageTable *table, uint64_t offset)
{
    uint64_t address = table->entries[offset];

    if (!(address & PAGING_FLAG_PRESENT))
    {
        return NULL;
    }

    return (struct PageTable *)TranslateToHighHalfMemoryAddress(address & PAGE_ADDRESS_MASK);
}

/**
 * @brief Duplicate a page and all subpages
 * @param * self
 * @param entry Address of the entry to duplicate
 * @param level Level of duplicate ( 0 for first level 1 for second level etc )
 * @return New address of the entry
 */
static inline uint64_t DuplicateRecursive(struct PageTable *self, uint64_t entry, uint64_t level)
{
    const uint64_t flags = PAGING_FLAG_PRESENT | PAGING_FLAG_USER_ACCESSIBLE | PAGING_FLAG_WRITABLE;

    uint64_t *virt = (uint64_t *)TranslateToHighHalfMemoryAddress((entry & ~PAGE_FLAG_MASK));
    uint64_t newPage = PagingGetFreeFrame();
    uint64_t *newVirtual = (uint64_t *)TranslateToHighHalfMemoryAddress(newPage);

    PagingMapMemory(self, newVirtual, (void *)newPage, flags);

    memset(newVirtual, 0, 0x1000);

    if (level == 0)
    {
        memcpy(newVirtual, (void *)virt, 0x1000);
    }
    else
    {
        for (uint64_t i = 0; i < 512; i++)
        {
            if (virt[i] & PAGING_FLAG_PRESENT)
            {
                newVirtual[i] = DuplicateRecursive(self, virt[i], level - 1);
            }
        }
    }

    return newPage | (entry & PAGE_FLAG_MASK);
}

/**
 * @brief Maps a region of virtual memory to P4.
 * @param * p4
 * @param * virtualMemory
 * @param flags Flags to control mapping
 */
void PagingIdentityMap(struct PageTable *p4, void *virtualMemory, uint64_t flags)
{

    if (k_mode.addr_debug == 2)
    {
        printf_("%s\n", "-------------------------------------------");
        printf_("%s\n", " DEBUG: Identity Map Adresse (Direct from args)");
        printf_("%s\n", "-------------------------------------------");
    }
    PagingMapMemory(p4, virtualMemory, virtualMemory, flags);
    if (k_mode.addr_debug == 2)
    {
        printf_("%s\n", "-------------------------------------------");
        printf_("%s\n", "-------------------------------------------");
    }
}

/**
 * @brief Maps a virtual address to a physical address.
 * @param * p4
 * @param * virtualMemory
 * @param * physicalMemory
 * @param flags The paging flags that should be set
 */
void PagingMapMemory(struct PageTable *p4, void *virtualMemory, void *physicalMemory, uint64_t flags)
{

    if (k_mode.addr_debug == 2)
    {

        printf_("%s", "Virtual: ");
        printf_("0x%llx\n", virtualMemory);
        printf_("%s", "Physical: ");
        printf_("0x%llx\n", physicalMemory);
    }

    uint64_t higherPermissions = PAGING_FLAG_WRITABLE | PAGING_FLAG_USER_ACCESSIBLE;

    struct PageTableOffset offset = VirtualAddressToOffsets(virtualMemory);

    struct PageTable *p4Virtual = (struct PageTable *)TranslateToHighHalfMemoryAddress((uint64_t)p4);

    struct PageTable *pdp = GetOrAllocEntry(p4Virtual, offset.p4Offset, higherPermissions);
    struct PageTable *pd = GetOrAllocEntry(pdp, offset.pdpOffset, higherPermissions);
    struct PageTable *pt = GetOrAllocEntry(pd, offset.pdOffset, higherPermissions);

    pt->entries[offset.ptOffset] = (uint64_t)physicalMemory | flags | PAGING_FLAG_PRESENT;
}

/**
 * @brief PagingPhysicalMemory is used to determine if a virtual address is paged.
 * @param * p4
 * @param * virtualMemory
 * @return The pointer to the page that was paged
 */
void *PagingPhysicalMemory(struct PageTable *p4, void *virtualMemory)
{
    struct PageTableOffset offset = VirtualAddressToOffsets(virtualMemory);

    struct PageTable *p4Virtual = (struct PageTable *)TranslateToHighHalfMemoryAddress((uint64_t)p4);

    struct PageTable *pdp = GetOrNullifyEntry(p4Virtual, offset.p4Offset);

    if (pdp == NULL)
    {
        return NULL;
    }

    struct PageTable *pd = GetOrNullifyEntry(pdp, offset.pdpOffset);

    if (pd == NULL)
    {
        return NULL;
    }

    struct PageTable *pt = GetOrNullifyEntry(pd, offset.pdOffset);

    if (pt == NULL)
    {
        return NULL;
    }

    return (void *)(pt->entries[offset.ptOffset] & ~(PAGE_FLAG_MASK));
}

/**
 * @brief Unmaps a page table that was mapped with PagingMapMemory.
 * @param * p4
 * @param * virtualMemory
 * @return Nothing. Side effects : None
 */
void PagingUnmapMemory(struct PageTable *p4, void *virtualMemory)
{
    struct PageTableOffset offset = VirtualAddressToOffsets(virtualMemory);

    struct PageTable *p4Virtual = (struct PageTable *)TranslateToHighHalfMemoryAddress((uint64_t)p4);
    struct PageTable *pdp = GetOrNullifyEntry(p4Virtual, offset.p4Offset);

    if (pdp == NULL)
    {
        return;
    }

    struct PageTable *pd = GetOrNullifyEntry(pdp, offset.pdpOffset);

    if (pd == NULL)
    {
        return;
    }

    struct PageTable *pt = GetOrNullifyEntry(pd, offset.pdOffset);

    if (pt == NULL)
    {
        return;
    }

    pt->entries[offset.ptOffset] = 0;
}

/**
 * @brief Duplicate page table and all pages present in it
 * @param * p4
 * @param * newTable
 */
void PagingDuplicate(struct PageTable *p4, struct PageTable *newTable)
{
    struct PageTable *p4Virtual = (struct PageTable *)TranslateToHighHalfMemoryAddress((uint64_t)p4);

    memset(newTable, 0, 0x1000);

    for (uint64_t i = 0; i < 256; i++)
    {
        uint64_t entry = p4Virtual->entries[i];

        // printf_("0x%llx\n", entry);

        if (entry & PAGING_FLAG_PRESENT)
        {
            newTable->entries[i] = DuplicateRecursive(p4, entry, 3);
        }
    }

    for (uint64_t i = 256; i < 512; i++)
    {
        newTable->entries[i] = p4Virtual->entries[i];
    }
}
