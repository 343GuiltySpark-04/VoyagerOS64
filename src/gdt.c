#include "include/gdt.h"
#include "include/global_defs.h"
#include <stdint.h>
#include "include/printf.h"
#include "include/tss.h"
#include "include/lock.h"

extern void breakpoint();
extern void serial_debug(int);
extern void halt();

uint8_t TssStack[0x100000];
uint8_t ist1Stack[0x100000];
uint8_t ist2Stack[0x100000];

uint64_t rsp0;

struct TSS tss = {0};

ALIGN_4K struct GDT gdt = {
    {.limit_low = 0,
     .base_low = 0,
     .base_middle = 0,
     .access_flag = 0x00,
     .limit_flags = 0x00,
     .base_high = 0}, // null

    {
        .limit_low = 0xffff,
        .base_low = 0,
        .base_middle = 0,
        .access_flag = GDTAccess16Code,
        .limit_flags = 0b00000000,
        .base_high = 0 // kernel 16 bit code segment

    },

    {

        .limit_low = 0xffff,
        .base_low = 0,
        .base_middle = 0,
        .access_flag = GDTAccess16Data,
        .limit_flags = 0b00000000,
        .base_high = 0 // kernel 16 bit data segment

    },

    {

        .limit_low = 0xffff,
        .base_low = 0,
        .base_middle = 0,
        .access_flag = GDTAccess32Code,
        .limit_flags = 0b11001111,
        .base_high = 0 // kernel 32 bit code segment

    },

    {

        .limit_low = 0xffff,
        .base_low = 0,
        .base_middle = 0,
        .access_flag = GDTAccess32Data,
        .limit_flags = 0b11001111,
        .base_high = 0 // kernel 32 bit data segment

    },

    {.limit_low = 0,
     .base_low = 0,
     .base_middle = 0,
     .access_flag = GDTAccessKernelCode,
     .limit_flags = 0xA0,
     .base_high = 0}, // kernel 64 bit code segment
    {
        .limit_low = 0,
        .base_low = 0,
        .base_middle = 0,
        .access_flag = GDTAccessKernelData,
        .limit_flags = 0x80,
        .base_high = 0}, // kernel 64 bit data segment
    {
        .limit_low = 0,
        .base_low = 0,
        .base_middle = 0,
        .access_flag = 0x00,
        .limit_flags = 0x00,
        .base_high = 0}, // user null
    {
        .limit_low = 0,
        .base_low = 0,
        .base_middle = 0,
        .access_flag = GDTAccessUserData,
        .limit_flags = 0x80,
        .base_high = 0}, // user data segment
    {
        .limit_low = 0,
        .base_low = 0,
        .base_middle = 0,
        .access_flag = GDTAccessUserCode,
        .limit_flags = 0xA0,
        .base_high = 0}, // user code segment
    {
        .length = 104,
        .flags = 0b10001001,
    }, // TSS
};

struct GDT_Desc desc = {

    .size = sizeof(gdt) - 1,
    .offset = (uint64_t)&gdt};

/**
 * @brief Reload GDT. This is called from reload.
 */
void gdt_reload(void)
{

    asm volatile(
        "lgdt %0\n\t"
        "push $0x28\n\t"
        "lea 1f(%%rip), %%rax\n\t"
        "push %%rax\n\t"
        "lretq\n\t"
        "1:\n\t"
        "mov $0x30, %%eax\n\t"
        "mov %%eax, %%ds\n\t"
        "mov %%eax, %%es\n\t"
        "mov %%eax, %%fs\n\t"
        "mov %%eax, %%gs\n\t"
        "mov %%eax, %%ss\n\t"
        :
        : "m"(desc)
        : "rax", "memory");
}

/**
 * @brief Loads TSS into GDT.
 * @param * tss
 */
void gdt_load_tss(struct TSS *tss)
{

    uintptr_t addr = (uintptr_t)tss;

    static spinlock_t lock = SPINLOCK_INIT;

    spinlock_acquire(&lock);

    gdt.tss = (struct TSS_Entry){
        .length = 104,
        .base_low = (uint16_t)addr,
        .base_mid = (uint8_t)(addr >> 16),
        .flags = 0b10001001,
        .base_high = (uint8_t)(addr >> 24),
        .base_up = (uint32_t)(addr >> 32),
    };

    asm volatile("ltr %0"
                 :
                 : "rm"((uint16_t)0x58)
                 : "memory");

    spinlock_release(&lock);
}

/**
 * @brief Stage 1 of GDT loading
 */
void LoadGDT_Stage1()
{
    uint64_t address = (uint64_t)&tss;

    gdt.tss = (struct TSS_Entry){
        .length = 104,
        .base_low = (uint16_t)address,
        .base_mid = (uint8_t)(address >> 16),
        .flags = 0b10001001,
        .base_high = (uint8_t)(address >> 24),
        .base_up = (uint32_t)(address >> 32),
    };

    printf_("%s\n", "------------------------------------");
    printf_("%s\n", "|              GDT INFO            |");
    printf_("%s\n", "------------------------------------");
    printf_("%s\n", "GDT Offsets as follows: ");
    printf_("%s", "GDT NULL Segment: ");
    printf_("0x%llx\n", (uint64_t)&gdt.null - (uint64_t)&gdt);
    printf_("%s", "GDT 16 Bit Code Segment: ");
    printf_("0x%llx\n", (uint64_t)&gdt.seg_16_code - (uint64_t)&gdt);
    printf_("%s", "GDT 16 Bit Data Segment: ");
    printf_("0x%llx\n", (uint64_t)&gdt.seg_16_data - (uint64_t)&gdt);
    printf_("%s", "GDT 32 Bit Code Segment: ");
    printf_("0x%llx\n", (uint64_t)&gdt.seg_32_code - (uint64_t)&gdt);
    printf_("%s", "GDT 32 Bit Data Segment: ");
    printf_("0x%llx\n", (uint64_t)&gdt.seg_32_data - (uint64_t)&gdt);
    printf_("%s", "GDT Kernel Code Segment: ");
    printf_("0x%llx\n", (uint64_t)&gdt.kernelCS - (uint64_t)&gdt);
    printf_("%s", "GDT Kernel Data Segment: ");
    printf_("0x%llx\n", (uint64_t)&gdt.kernelData - (uint64_t)&gdt);
    printf_("%s", "GDT User NULL Segment: ");
    printf_("0x%llx\n", (uint64_t)&gdt.userNull - (uint64_t)&gdt);
    printf_("%s", "GDT User Code Segment: ");
    printf_("0x%llx\n", (uint64_t)&gdt.userCode - (uint64_t)&gdt);
    printf_("%s", "GDT User Data Segment: ");
    printf_("0x%llx\n", (uint64_t)&gdt.userData - (uint64_t)&gdt);
    printf_("%s", "GDT TSS Segment: ");
    printf_("0x%llx\n", (uint64_t)&gdt.tss - (uint64_t)&gdt);
    printf_("%s\n", "------------------------------------");
    printf_("%s\n", "Expected GDTR Data as Follows: ");
    printf_("%s", "GDTR Size: ");
    printf_("0x%llx\n", (uint16_t)&desc.size);
    printf_("%s", "GDTR Offset: ");
    printf_("0x%llx\n", (uint64_t)&desc.offset);
    printf_("%s\n", "------------------------------------");
    tss.rsp0 = (uint64_t)TssStack + sizeof(TssStack);
    tss.ist1 = (uint64_t)ist1Stack + sizeof(ist1Stack);

    rsp0 = tss.rsp0;

    printf_("0x%llx\n", tss.rsp0);

    __asm__ volatile("lgdt %0"
                     :
                     : "m"(desc));

    __asm__ volatile("push $0x28\n"
                     "lea 1f(%%rip), %%rax\n"
                     "push %%rax\n"
                     "lretq\n"
                     "1:\n"
                     :
                     :
                     : "rax", "memory");

    __asm__ volatile("mov %0, %%ds\n"
                     "mov %0, %%es\n"
                     "mov %0, %%gs\n"
                     "mov %0, %%fs\n"
                     "mov %0, %%ss\n"
                     :
                     : "a"((uint16_t)0x30));

    __asm__ volatile("ltr %0"
                     :
                     : "a"((uint16_t)GDTTSSSegment));

    // breakpoint();
}