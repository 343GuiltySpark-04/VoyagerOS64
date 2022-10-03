#include "include/gdt.h"
#include "include/global_defs.h"
#include <stdint.h>

__attribute__((aligned(0x1000))) struct GDT_Entry null_seg = {

    .base = 0,
    .limit = 0x00000000,
    .access_byte = 0x00,
    .flags = 0x00

};

struct GDT_Entry Kernel_cs = {

    .base = 0,
    .limit = 0xFFFFF,
    .access_byte = GDTAccessKernelCode,
    .flags = 0xA

};

struct GDT_Entry Kernel_ds = {

    .base = 0,
    .limit = 0xFFFFF,
    .access_byte = GDTAccessKernelData,
    .flags = 0xC

};

struct GDT_Entry User_cs = {

    .base = 0,
    .limit = 0xFFFFF,
    .access_byte = GDTAccessUserCode,
    .flags = 0xA

};

struct GDT_Entry User_ds = {

    .base = 0,
    .limit = 0xFFFFF,
    .access_byte = GDTAccessUserData,
    .flags = 0xC

};