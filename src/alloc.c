#include <stdint.h>
#include <stddef.h>
#include "include/string.h"
#include "include/paging/frameallocator.h"
#include "include/paging/paging.h"

extern void *liballoc_alloc(size_t pages)
{

    void *ptr = frame_request_multiple(pages);

    if (ptr == NULL)
    {

        return NULL;
    }

    void *realPtr = (void *)TranslateToHighHalfMemoryAddress((uint64_t)ptr);

    return realPtr;
}

extern int liballoc_free(void *ptr, size_t pages)
{

    void *realPtr = (void *)TranslateToPhysicalMemoryAddress((uint64_t)ptr);

    frame_free_multiple(realPtr, pages);

    return 0;
}