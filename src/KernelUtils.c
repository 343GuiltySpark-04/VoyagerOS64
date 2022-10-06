#include "include/KernelUtils.h"

void NMI_enable()
{
    outb(0x70, inb(0x70) & 0x7F);
    inb(0x71);
}

void NMI_disable()
{
    outb(0x70, inb(0x70) | 0x80);
    inb(0x71);
}
