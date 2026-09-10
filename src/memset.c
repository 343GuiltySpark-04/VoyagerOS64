#include "include/string.h"

/**
 * @brief Fill a memory region with one byte value.
 * @param bufptr Destination memory region.
 * @param value Value whose low eight bits are written to each byte.
 * @param size Number of bytes to set.
 * @return @p bufptr.
 */
void *memset(void *bufptr, int value, size_t size)
{
    unsigned char *buf = (unsigned char *) bufptr;
    for (size_t i = 0; i < size; i++)
        buf[i] = (unsigned char) value;
    return bufptr;
}
