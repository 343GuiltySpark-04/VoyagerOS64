#include "../include/acpi/acpi.h"
#include "../include/acpi/madt.h"
#include "../include/cpuUtils.h"
#include "../include/global_defs.h"
#include "../include/limine.h"
#include "../include/paging/frameallocator.h"
#include "../include/paging/paging.h"
#include "../include/printf.h"
#include "../include/string.h"
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

extern void halt();

static volatile struct limine_rsdp_request rsdp_req = {

    .id = LIMINE_RSDP_REQUEST, .revision = 0

};

struct sdt *sdt;

struct rsdp
{

    char signature[8];
    uint8_t checksum;
    char oem_id[6];
    uint8_t revision;
    uint32_t rsdt_addr;
    uint32_t length;
    uint64_t xsdt_addr;
    uint8_t ext_checksum;
    char reserved[3];
};

struct rsdt
{

    struct sdt;
    char data[];
};

static struct rsdp *rsdp = NULL;
static struct rsdt *rsdt = NULL;

/**
* @brief Check if we should use XSDT.
* @return True if XSDT should be used
*/
static inline bool use_xsdt(void)
{
    return rsdp->revision >= 2 && rsdp->xsdt_addr != 0;
}

/**
* @brief Initialize ACPI subsystem This is called at boot time
*/
void acpi_init(void)
{
    struct limine_rsdp_response *rsdp_resp = rsdp_req.response;

    if (rsdp_resp == NULL || rsdp_resp->address == NULL)
    {

        printf_("%s\n", "!!!!!!!!!!!!KERNEL PANIC!!!!!!!!!!!!!");
        printf_("%s\n", " ACPI Not Supported by This Machine!");
        printf_("%s\n", "!!!!!!!!!!!!!KERNEL PANIC!!!!!!!!!!!!!");
        halt();
    }
    else if (has_ACPI == false)
    {

        printf_("%s\n", "!!!!!!!!!!!!KERNEL PANIC!!!!!!!!!!!!!");
        printf_("%s\n", " ACPI Not Supported by This Machine!");
        printf_("%s\n", "!!!!!!!!!!!!!KERNEL PANIC!!!!!!!!!!!!!");
        halt();
    }

    rsdp = rsdp_resp->address;

    if (use_xsdt())
    {
        rsdt = (struct rsdt *)(rsdp->xsdt_addr + HIGHER_HALF_MEMORY_OFFSET);
    }
    else
    {
        rsdt = (struct rsdt *)((uint64_t)rsdp->rsdt_addr + HIGHER_HALF_MEMORY_OFFSET);
    }

    printf_("acpi: Revision: %lu\n", rsdp->revision);
    printf_("acpi: Uses XSDT? %s\n", use_xsdt() ? "true" : "false");
    printf_("acpi: RSDT at 0x%llx\n", rsdt);

    struct sdt *fadt = acpi_find_sdt("FACP", 0);
    if (fadt != NULL && fadt->length >= 116)
    {
        uint32_t fadt_flags = *((uint32_t *)fadt + 28);

        if ((fadt_flags & (1 << 20)) != 0)
        {
            printf_("%s\n", "!!!!!!!!!!!!!!!!!KERNEL PANIC!!!!!!!!!!!!!!!!!!");
            printf_("%s\n", " VoyagerOS64 Does Not Support HW Reduced ACPI");
            printf_("%s\n", "!!!!!!!!!!!!!!!!!KERNEL PANIC!!!!!!!!!!!!!!!!!!");
            halt();
        }
    }

    madt_init();
}

/**
* @brief Locates SDT by signature
* @param char
* @param index Index of entry to search.
* @return Pointer to the entry or NULL if not found
*/
void *acpi_find_sdt(const char signature[static 4], size_t index)
{
    size_t entry_count =
        (rsdt->length - sizeof(struct sdt)) / (use_xsdt() ? 8 : 4);

    for (size_t i = 0; i < entry_count; i++)
    {
        struct sdt *sdt = NULL;
        if (use_xsdt())
        {
            sdt = (struct sdt *)(*((uint64_t *)rsdt->data + i) +
                                 HIGHER_HALF_MEMORY_OFFSET);
        }
        else
        {
            sdt = (struct sdt *)(*((uint32_t *)rsdt->data + i) +
                                 HIGHER_HALF_MEMORY_OFFSET);
        }

        if (memcmp(sdt->signature, signature, 4) != 0)
        {
            continue;
        }

        if (index > 0)
        {
            index--;
            continue;
        }

        printf_("acpi: Found '%S' at 0x%llx, length=%lu\n", signature, 4, sdt,
                sdt->length);
        return sdt;
    }

    printf_("acpi: Could not find '%S'\n", signature, 4);
    return NULL;
}