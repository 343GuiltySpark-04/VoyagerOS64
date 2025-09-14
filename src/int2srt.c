/**
 * Copyright (c) 2025 Tristan Adams
 *
 * This software is released under the MIT License.
 * https://opensource.org/licenses/MIT
 */

#include <stdint.h>

char int_to_str(int value, char *buffer)
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
        buffer[i++] = '0' + digit; // convert int → char
        value /= 10;
    } while (value > 0);

    if (is_negative)
    {
        buffer[i++] = '-';
    }

    buffer[i] = '\0';

    // reverse string in-place
    for (int j = 0; j < i / 2; j++)
    {
        char tmp          = buffer[j];
        buffer[j]         = buffer[i - j - 1];
        buffer[i - j - 1] = tmp;
    }

    return buffer;
}
