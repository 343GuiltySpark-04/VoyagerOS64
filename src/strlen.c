#include "include/string.h"

/**
* @brief Returns the length of a string.
* @param char
* @return The length of the string
*/
size_t strlen(const char *str)
{
    size_t len = 0;
    while (str[len])
        len++;
    return len;
}
