#include "include/serial.h"
#include "include/printf.h"

void serial_print(const char *str)
{
    char *p = (char *)str;

    while (*p)
    {
        serial_debug(*p++);
    }
}

void serial_print_line(const char *str)
{
    char *p = (char *)str;

    while (*p)
    {
        serial_debug(*p++);
    }

    serial_debug('\n');
}