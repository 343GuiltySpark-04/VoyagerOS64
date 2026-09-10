#include "include/string.h"

/**
 * @brief Copy bytes from a non-overlapping source region to a destination.
 * @param dstptr Destination memory region.
 * @param srcptr Source memory region.
 * @param size Number of bytes to copy.
 * @return @p dstptr.
 */
void *memcpy(void *restrict dstptr, const void *restrict srcptr, size_t size)
{
    unsigned char       *dst = (unsigned char *) dstptr;
    const unsigned char *src = (const unsigned char *) srcptr;
    for (size_t i = 0; i < size; i++)
        dst[i] = src[i];
    return dstptr;
}
