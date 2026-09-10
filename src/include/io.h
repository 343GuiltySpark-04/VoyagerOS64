/**
 * @file io.h
 * @brief x86 port-I/O primitives.
 * @ingroup drivers
 */
#pragma once
#ifndef _IO_H
#define _IO_H

#include <stdint.h>

/**
 * @brief Read one byte from an x86 I/O port.
 * @param port I/O-port address.
 * @return Byte read from the requested port.
 */
uint8_t inb(uint16_t port);

/**
 * @brief Write one byte to an x86 I/O port.
 * @param port I/O-port address.
 * @param val Byte to write.
 */
void outb(uint16_t port, uint8_t val);

/**
 * @brief Perform the traditional short I/O delay used around legacy devices.
 *
 * This helper is used by hardware such as the PIC/PIT/PS2 paths where ordered
 * or slightly delayed port transactions are required by the device interface.
 */
void io_wait(void);

#endif
