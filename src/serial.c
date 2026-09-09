#include "include/printf.h"
#include "include/serial.h"
#include "include/string.h"

/**
 * @brief Print a timestamped string to the serial port.
 * @param str String to print.
 */
void serial_print(const char *str)
{
    char timestamp_buf[32];
    u64_to_str(get_ts(), timestamp_buf);

    serial_debug('[');
    for (char *p = timestamp_buf; *p; p++)
        serial_debug(*p);
    serial_debug(']');
    serial_debug(' ');

    for (const char *p = str; *p; p++)
        serial_debug(*p);
}

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
 * @brief Print a timestamped string followed by a newline.
 * @param str String to print.
 */
void serial_print_line(const char *str)
{
    char timestamp_buf[32];
    u64_to_str(get_ts(), timestamp_buf);

    serial_debug('[');
    for (char *p = timestamp_buf; *p; p++)
        serial_debug(*p);
    serial_debug(']');
    serial_debug(' ');

    for (const char *p = str; *p; p++)
        serial_debug(*p);

    serial_debug('\n');
}
