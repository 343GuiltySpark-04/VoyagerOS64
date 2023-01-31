#include "include/io.h"
#include <stdint.h>

/**
* @brief Read a byte from a port
* @param port The port to read from
* @return The byte read from the
*/
uint8_t inb(uint16_t port)
{
    uint8_t ret;
    asm volatile("inb %1, %0"
                 : "=a"(ret)
                 : "Nd"(port));
    return ret;
}

/**
* @brief Write a byte to a port.
* @param port The port to write to.
* @param val The byte to write
*/
void outb(uint16_t port, uint8_t val)
{
    asm volatile("outb %1, %0" ::"dN"(port), "a"(val));
}

/**
* @brief Wait for I / O to complete
* @return 0 on success non -
*/
void io_wait(void)
{
    outb(0x80, 0);
}