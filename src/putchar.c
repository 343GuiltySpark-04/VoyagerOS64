#include "include/printf.h"
#include "include/serial.h"

void _putchar(char character)
{
    // print char to serial
    serial_debug(character);
}