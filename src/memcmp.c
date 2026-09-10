#include "include/string.h"

/**
 * @brief Compare two memory blocks byte by byte.
 * @param aptr Pointer to the first memory block.
 * @param bptr Pointer to the second memory block.
 * @param size Number of bytes to compare.
 * @return Negative when the first differing byte in @p aptr is smaller, zero
 *         when all requested bytes are equal, or positive when it is larger.
 */
int memcmp(const void *aptr, const void *bptr, size_t size)
{
    const unsigned char *a = (const unsigned char *) aptr;
    const unsigned char *b = (const unsigned char *) bptr;
    for (size_t i = 0; i < size; i++)
    {
        if (a[i] < b[i])
            return -1;
        else if (b[i] < a[i])
            return 1;
    }
    return 0;
}
