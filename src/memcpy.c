#include "include/string.h"

/**
* @brief Copies size bytes from srcptr to dstptr.
* @param * restrict dstptr
* @param void
* @param size Number of bytes to copy.
* @return Pointer to destination buffer (same as dstptr)
*/
void *memcpy(void *restrict dstptr, const void *restrict srcptr, size_t size)
{
    unsigned char *dst = (unsigned char *)dstptr;
    const unsigned char *src = (const unsigned char *)srcptr;
    for (size_t i = 0; i < size; i++)
        dst[i] = src[i];
    return dstptr;
}
