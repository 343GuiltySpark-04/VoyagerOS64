/**
 * @file pic.h
 * @brief Legacy 8259 PIC programming interface.
 * @ingroup interrupts
 */
#pragma once
#include "stdint.h"

#define PIC_MASTER 0x20
#define PIC_SLAVE 0xA0
#define PIC_MASTER_COMMAND PIC_MASTER
#define PIC_MASTER_DATA (PIC_MASTER + 1)
#define PIC_SLAVE_COMMAND PIC_SLAVE
#define PIC_SLAVE_DATA (PIC_SLAVE + 1)
#define PIC_EOI 0x20

#define PIC_ICW1_ICW4 0x01
#define PIC_ICW1_SINGLE 0x02
#define PIC_ICW1_INTERVAL4 0x04
#define PIC_ICW1_LEVEL 0x08
#define PIC_ICW1_INIT 0x10

#define PIC_ICW4_8086 0x01
#define PIC_ICW4_AUTO 0x02
#define PIC_ICW4_BUF_SLAVE 0x08
#define PIC_ICW4_BUF_MASTER 0x0C

/**
 * @brief Initialize/remap the legacy PICs for Voyager's interrupt layout.
 */
void pic_enable(void);

/**
 * @brief Remap PIC IRQ0 to a new IDT vector base.
 * @param irq_offset Vector number corresponding to IRQ0 after remapping.
 */
void pic_remap_offsets(uint8_t irq_offset);

/**
 * @brief Mask one legacy PIC IRQ line.
 * @param irq IRQ number in the range handled by the cascaded PICs.
 */
void pic_mask_irq(uint8_t irq);

/**
 * @brief Unmask one legacy PIC IRQ line.
 * @param irq IRQ number in the range handled by the cascaded PICs.
 */
void pic_unmask_irq(uint8_t irq);

/**
 * @brief Send end-of-interrupt acknowledgement for one legacy PIC IRQ.
 * @param irq Hardware IRQ number, not an arbitrary IDT vector.
 *
 * The generic interrupt dispatcher only sends PIC EOI for vectors belonging to
 * the remapped hardware IRQ range. Dynamic/software vectors must not be passed
 * through PIC acknowledgement merely because they used the same dispatcher.
 */
void pic_send_eoi(uint8_t irq);
