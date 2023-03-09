#include "include/string.h"

/**
* @brief Copies size bytes from srcptr to dstptr.
* @param * dstptr
* @param void
* @param size Number of bytes to copy.
* @return Pointer to destination memory (dstptr)
*/
void *memmove(void *dstptr, const void *srcptr, size_t size)
{
    unsigned char *dst = (unsigned char *)dstptr;
    const unsigned char *src = (const unsigned char *)srcptr;
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
