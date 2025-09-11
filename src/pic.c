#include "include/pic.h"
#include "include/io.h"

/**
* @brief Mask an IRQ in the PIC.
* @param irq The IRQ number to mask
*/
void pic_mask_irq(uint8_t irq)
{
    uint16_t port;
    uint8_t masks;

    if (irq < 8)
    {
        port = PIC_MASTER_DATA;
    }
    else
    {
        port = PIC_SLAVE_DATA;
        irq -= 8;
    }

    masks = inb(port);
    masks |= (1 << irq);
    outb(port, masks);
}

/**
* @brief Unmask an IRQ in the PIC.
* @param irq The IRQ number to
*/
void pic_unmask_irq(uint8_t irq)
{
    uint16_t port;
    uint8_t masks;

    if (irq < 8)
    {
        port = PIC_MASTER_DATA;
    }
    else
    {
        port = PIC_SLAVE_DATA;
        irq -= 8;
    }

    masks = inb(port);
    masks &= ~(1 << irq);
    outb(port, masks);
}

/**
* @brief Change the I / O addresses of the PIC to offset.
* @param offset Offset to be remapped
*/
void pic_remap_offsets(uint8_t offset)
{
    uint8_t master_mask, slave_mask;

    master_mask = inb(PIC_MASTER_DATA);
    slave_mask = inb(PIC_SLAVE_DATA);

    outb(PIC_MASTER_COMMAND, PIC_ICW1_INIT | PIC_ICW1_ICW4);
    outb(PIC_SLAVE_COMMAND, PIC_ICW1_INIT | PIC_ICW1_ICW4);

    outb(PIC_MASTER_DATA, offset);
    outb(PIC_SLAVE_DATA, offset + 0x08);

    outb(PIC_MASTER_DATA, 0x04);
    outb(PIC_SLAVE_DATA, 0x02);

    outb(PIC_MASTER_DATA, PIC_ICW4_8086);
    outb(PIC_SLAVE_DATA, PIC_ICW4_8086);

    outb(PIC_MASTER_DATA, master_mask);
    outb(PIC_SLAVE_DATA, slave_mask);
}

/**
* @brief Send End Of Interrupt.
* @param irq IRQ number to send EOI for
*/
void pic_send_eoi(uint8_t irq)
{
    if (irq >= 8)
        outb(PIC_SLAVE_COMMAND, PIC_EOI);
    outb(PIC_MASTER_COMMAND, PIC_EOI);
}

/**
* @brief Enable PIC interrupts. This is called by the interrupt handler to turn on all IRQs
*/
void pic_enable()
{
    pic_remap_offsets(0x20);
    for (uint8_t irq = 0; irq < 16; irq++)
        pic_mask_irq(irq);
}
