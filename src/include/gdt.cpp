#include "gdt.hpp"
#include <stdint.h>

uint8_t bootstrapTssStack[0x100000];
uint8_t bootstrapist1Stack[0x100000];
uint8_t bootstrapist2Stack[0x100000];

TSS bootstrapTSS = {0};

__attribute__((aligned(0x1000)))
GDT bootstrapGDT = {
    {.limitLow = 0,
     .baseLow = 0,
     .baseMiddle = 0,
     .accessFlag = 0x00,
     .limitFlags = 0x00,
     .baseHigh = 0}, // null
    {
        .limitLow = 0,
        .baseLow = 0,
        .baseMiddle = 0,
        .accessFlag = GDTAccessKernelCode,
        .limitFlags = 0xA0,
        .baseHigh = 0}, // kernel code segment
    {
        .limitLow = 0,
        .baseLow = 0,
        .baseMiddle = 0,
        .accessFlag = GDTAccessKernelData,
        .limitFlags = 0x80,
        .baseHigh = 0}, // kernel data segment
    {
        .limitLow = 0,
        .baseLow = 0,
        .baseMiddle = 0,
        .accessFlag = 0x00,
        .limitFlags = 0x00,
        .baseHigh = 0}, // user null
    {
        .limitLow = 0,
        .baseLow = 0,
        .baseMiddle = 0,
        .accessFlag = GDTAccessUserData,
        .limitFlags = 0x80,
        .baseHigh = 0}, // user data segment
    {
        .limitLow = 0,
        .baseLow = 0,
        .baseMiddle = 0,
        .accessFlag = GDTAccessUserCode,
        .limitFlags = 0xA0,
        .baseHigh = 0}, // user code segment
    {
        .length = 104,
        .flags = 0b10001001,
    }, // TSS
};

GDTDescriptor bootstrapGDTR = {
    .size = sizeof(bootstrapGDT) - 1,
    .offset = (uint64_t)&bootstrapGDT};