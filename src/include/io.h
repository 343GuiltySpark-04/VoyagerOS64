#pragma once
#include <stdint.h>
#include "sched.h"

#ifndef _IO_H
#define _IO_H

/** inb:
 *  Read a byte from an I/O port.
 *
 *  @param  port The address of the I/O port
 *  @return      The read byte
 */
uint8_t inb(uint16_t port);

/** outb:
 *  Sends the given data to the given I/O port. Defined in io.s
 *
 *  @param port The I/O port to send the data to
 *  @param data The data to send to the I/O port
 */
void outb(uint16_t port, uint8_t data);

void io_wait(void);

void stdin(char flags, ...);

void stdout(char flags, struct active_tube *a, struct standby_tube *s);



// these are exposed temporarly to test functionality will been called via a wrapper at release/when finished!

char pop_io(struct active_tube *a, struct standby_tube *s, uint64_t pid);

void push_io(struct active_tube *a, struct standby_tube *s, uint64_t pid, ...);

#endif