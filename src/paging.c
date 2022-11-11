#include "include/paging/paging.h"
#include "include/paging/frameallocator.h"
#include "include/string.h"
#include "include/printf.h"

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

void *OffsetToVirtualAddress(struct PageTableOffset offset)
{
    uint64_t address = 0;

    address |= offset.p4Offset << 39;
    address |= offset.pdpOffset << 30;
    address |= offset.pdOffset << 21;
    address |= offset.ptOffset << 12;

    return (void *)address;
}

uint64_t ReadCR3()
{
    uint64_t outValue = 0;

    asm("mov %%cr3, %0"
        : "=r"(outValue)
        :);

    return outValue;
    // no input
}

void WriteCR3(uint64_t value)
{
    asm("mov %0, %%cr3"
        : // no output
        : "r"(value));
}

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

static inline struct PageTable *GetOrNullifyEntry(struct PageTable *table, uint64_t offset)
{
    uint64_t address = table->entries[offset];

    if (!(address & PAGING_FLAG_PRESENT))
    {
        return NULL;
    }

    return (struct PageTable *)TranslateToHighHalfMemoryAddress(address & PAGE_ADDRESS_MASK);
}

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

void PagingIdentityMap(struct PageTable *p4, void *virtualMemory, uint64_t flags)
{
    PagingMapMemory(p4, virtualMemory, virtualMemory, flags);
}

void PagingMapMemory(struct PageTable *p4, void *virtualMemory, void *physicalMemory, uint64_t flags)
{
    uint64_t higherPermissions = PAGING_FLAG_WRITABLE | PAGING_FLAG_USER_ACCESSIBLE;

    struct PageTableOffset offset = VirtualAddressToOffsets(virtualMemory);

    struct PageTable *p4Virtual = (struct PageTable *)TranslateToHighHalfMemoryAddress((uint64_t)p4);

    struct PageTable *pdp = GetOrAllocEntry(p4Virtual, offset.p4Offset, higherPermissions);
    struct PageTable *pd = GetOrAllocEntry(pdp, offset.pdpOffset, higherPermissions);
    struct PageTable *pt = GetOrAllocEntry(pd, offset.pdOffset, higherPermissions);

    pt->entries[offset.ptOffset] = (uint64_t)physicalMemory | flags | PAGING_FLAG_PRESENT;
}

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
