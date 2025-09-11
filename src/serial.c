#include "include/serial.h"
#include "include/printf.h"

/**
* @brief Print a string to the serial port.
* @param str String to print.
* @return void
*/
void inline serial_print(const char *str)
{
    char *p = (char *)str;

    while (*p)
    {
        serial_debug(*p++);
    }
}

/**
* @brief Print a string ending with a new line break to serial port
* @param str String to print
* @return void
*/
void inline serial_print_line(const char *str)
{
    char *p = (char *)str;

    while (*p)
    {
        serial_debug(*p++);
    }

    serial_debug('\n');
}
