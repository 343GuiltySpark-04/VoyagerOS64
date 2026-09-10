#include "include/string.h"

/**
 * @brief Copy bytes between memory regions that may overlap.
 * @param dstptr Destination memory region.
 * @param srcptr Source memory region.
 * @param size Number of bytes to copy.
 * @return @p dstptr.
 */
void *memmove(void *dstptr, const void *srcptr, size_t size)
{
    unsigned char       *dst = (unsigned char *) dstptr;
    const unsigned char *src = (const unsigned char *) srcptr;
    if (dst < src)
    {
        for (size_t i = 0; i < size; i++)
            dst[i] = src[i];
    }
    else
    {
        for (size_t i = size; i != 0; i--)
            dst[i - 1] = src[i - 1];
    }
    return dstptr;
}
