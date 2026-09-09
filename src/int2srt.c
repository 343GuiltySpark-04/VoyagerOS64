/**
 * Copyright (c) 2025 Tristan Adams
 *
 * This software is released under the MIT License.
 * https://opensource.org/licenses/MIT
 */

#include <stdint.h>

char *int_to_str(int value, char *buffer)
{
    int is_negative = 0;
    if (value < 0)
    {
        is_negative = 1;
        value       = -value;
    }

    int i = 0;
    do
    {
        int digit   = value % 10;
        buffer[i++] = '0' + digit;
        value /= 10;
    } while (value > 0);

    if (is_negative)
        buffer[i++] = '-';

    buffer[i] = '\0';

    for (int j = 0; j < i / 2; j++)
    {
        char tmp          = buffer[j];
        buffer[j]         = buffer[i - j - 1];
        buffer[i - j - 1] = tmp;
    }

    return buffer;
}

void u64_to_str(uint64_t val, char *buf)
{
    char tmp[21];
    int  i = 0;
    do
    {
        tmp[i++] = '0' + (val % 10);
        val /= 10;
    } while (val > 0);
    buf[i] = '\0';
    for (int j = 0; j < i; j++)
        buf[j] = tmp[i - j - 1];
    buf[i] = '\0';
}
