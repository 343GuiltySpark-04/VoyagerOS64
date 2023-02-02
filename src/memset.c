#include "include/string.h"

/**
* @brief Sets memory to a value.
* @param * bufptr
* @param value Value to set each byte to.
* @param size Number of bytes to set.
* @return A pointer to the buffer
*/
void *memset(void *bufptr, int value, size_t size)
{
    unsigned char *buf = (unsigned char *)bufptr;
    for (size_t i = 0; i < size; i++)
        buf[i] = (unsigned char)value;
    return bufptr;
}
