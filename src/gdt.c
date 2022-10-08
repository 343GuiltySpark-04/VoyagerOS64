#include "include/gdt.h"
#include "include/global_defs.h"
#include <stdint.h>
#include "include/printf.h"

extern void breakpoint();
extern void serial_debug(int);

uint8_t TssStack[0x100000];
uint8_t ist1Stack[0x100000];
uint8_t ist2Stack[0x100000];

struct TSS tss = {0};

ALIGN_4K struct GDT gdt = {
    {.limit_low = 0,
     .base_low = 0,
     .base_middle = 0,
     .access_flag = 0x00,
     .limit_flags = 0x00,
     .base_high = 0}, // null
    {
        .limit_low = 0,
        .base_low = 0,
        .base_middle = 0,
        .access_flag = GDTAccessKernelCode,
        .limit_flags = 0xA0,
        .base_high = 0}, // kernel code segment
    {
        .limit_low = 0,
        .base_low = 0,
        .base_middle = 0,
        .access_flag = GDTAccessKernelData,
        .limit_flags = 0x80,
        .base_high = 0}, // kernel data segment
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

    printf_("%s\n", "--------------------------------------");
    printf_("%s\n", "GDT Offsets as follows: ");
    printf_("%s", "GDT NULL: ");
    printf_("0x%llx\n", (uint64_t)&gdt.null - (uint64_t)&gdt);
    printf_("%s", "GDT Kernel Code: ");
    printf_("0x%llx\n", (uint64_t)&gdt.kernelCS - (uint64_t)&gdt);
    printf_("%s", "GDT Kernel Data: ");
    printf_("0x%llx\n", (uint64_t)&gdt.kernelData - (uint64_t)&gdt);
    printf_("%s", "GDT User Code: ");
    printf_("0x%llx\n", (uint64_t)&gdt.userCode - (uint64_t)&gdt);
    printf_("%s", "GDT User Data: ");
    printf_("0x%llx\n", (uint64_t)&gdt.userData - (uint64_t)&gdt);
    printf_("%s", "GDT TSS: ");
    printf_("0x%llx\n", (uint64_t)&gdt.tss - (uint64_t)&gdt);
    printf_("%s\n", "--------------------------------------");

    tss.rsp0 = (uint64_t)TssStack + sizeof(TssStack);

    __asm__ volatile("lgdt %0"
                     :
                     : "m"(desc));

    __asm__ volatile("push $0x08\n"
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
                     : "a"((uint16_t)0x10));

    __asm__ volatile("ltr %0"
                     :
                     : "a"((uint16_t)GDTTSSSegment));

    // breakpoint();
}