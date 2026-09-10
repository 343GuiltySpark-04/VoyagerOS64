#include "include/string.h"
#include <stdint.h>

/**
 * @brief Convert a decimal string to an integer. (DEPRECATED)
 * @return Parsed value, or 0 if the string contains a non-decimal character.
 */
int str2int(char str[])
{
    int i      = 0;
    int result = 0;

    while (str[i] != '\0')
    {
        if (str[i] < '0' || str[i] > '9')
            return 0;

        result = result * 10 + (str[i] - '0');
        i++;
    }

    return result;
}

uint64_t str2int2(char *str)
{
    uint64_t res = 0;

    for (uint64_t i = 0; str[i] != '\0'; ++i)
    {
        uint64_t digit = str[i] - '0';
        res            = res * 10 + digit;
    }

    return res;
}
