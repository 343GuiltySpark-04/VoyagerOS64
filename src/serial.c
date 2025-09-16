#include "include/printf.h"
#include "include/serial.h"
#include "include/string.h"

static char serial_buff[64];

/**
 * @brief Print a string to the serial port.
 * @param str String to print.
 * @return void
 */
void inline serial_print(const char *str)
{
    // Convert timestamp to string using your helper
    char timestamp_buf[32]; // adjust size to whatever int_to_str needs
    int_to_str(get_ts(), timestamp_buf);

    // Print opening bracket
    serial_debug('[');

    // Print timestamp
    char *t = timestamp_buf;
    while (*t)
    {
        serial_debug(*t++);
    }

    // Print closing bracket and a space
    serial_debug(']');
    serial_debug(' ');

    // Now print the actual message
    const char *p = str;
    while (*p)
    {
        serial_debug(*p++);
    }

    // And finally a newline
    // serial_debug('\n');
}

/**
 * The function `serial_print_with_ts` prints a timestamp followed by a message
 * to a serial output.
 *
 * @param msg The `msg` parameter in the `serial_print_with_ts` function is a
 * pointer to a constant character string. This string contains the message that
 * you want to print along with a timestamp.
 */
void serial_print_with_ts(const char *msg)
{
    char ts[32];
    u64_to_str(get_ts(), ts);

    serial_debug('[');
    for (char *p = ts; *p; p++)
        serial_debug(*p);
    serial_debug(']');
    serial_debug(' ');

    for (const char *p = msg; *p; p++)
        serial_debug(*p);
    serial_debug('\n');
}

/**
 * @brief Print a string ending with a new line break to serial port
 * @param str String to print
 * @return void
 */
void inline serial_print_line(const char *str)
{
    // Convert timestamp to string using your helper
    char timestamp_buf[32]; // adjust size to whatever int_to_str needs
    int_to_str(get_ts(), timestamp_buf);

    // Print opening bracket
    serial_debug('[');

    // Print timestamp
    char *t = timestamp_buf;
    while (*t)
    {
        serial_debug(*t++);
    }

    // Print closing bracket and a space
    serial_debug(']');
    serial_debug(' ');

    // Now print the actual message
    const char *p = str;
    while (*p)
    {
        serial_debug(*p++);
    }

    // And finally a newline
    serial_debug('\n');
}
