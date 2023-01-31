#include "include/serial.h"
#include "include/printf.h"

/**
* @brief Print a string to the serial port.
* @param char
* @return Nothing. Side effects : None
*/
void serial_print(const char *str)
{
    char *p = (char *)str;

    while (*p)
    {
        serial_debug(*p++);
    }
}

/**
* @brief Print a line to serial port
* @param char
* @return void This function is used to print a
*/
void serial_print_line(const char *str)
{
    char *p = (char *)str;

    while (*p)
    {
        serial_debug(*p++);
    }

    serial_debug('\n');
}