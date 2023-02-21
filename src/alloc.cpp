#include <stdint.h>
#include <stddef.h>
#include "include/string.h"
#include "include/paging/frameallocator.h"
#include "include/paging/paging.h"
#include "include/lock.hpp"
#include "include/printf.h"

AtomicLock liballocLock;

/**
* @brief Locks the liballoc lock.
* @return Zero on success non - zero
*/
extern "C" int liballoc_lock()
{

    liballocLock.Lock();

    return 0;
}

/**
* @brief Unlock liballoc's lock
* @return Zero on success non - zero
*/
extern "C" int liballoc_unlock()
{

    liballocLock.Unlock();

    return 0;
}

/**
* @brief Allocate memory and translate it to high half memory
* @param pages number of pages to allocate
* @return pointer to allocated memory or NULL if
*/
extern "C" void *liballoc_alloc(size_t pages)
{

    void *ptr = frame_request_multiple(pages);

    if (ptr == NULL)
    {

        return NULL;
    }

    void *realPtr = (void *)TranslateToHighHalfMemoryAddress((uint64_t)ptr);

    return realPtr;
}

/**
* @brief Free memory allocated by liballoc.
* @param * ptr
* @param pages Number of pages to be freed.
* @return Zero on success non - zero on failure
*/
extern "C" int liballoc_free(void *ptr, size_t pages)
{

    printf_("%s", "INFO: liballoc_free ptr: ");
    printf_("0x%llx\n", ptr);

    void *realPtr = (void *)TranslateToPhysicalMemoryAddress((uint64_t)ptr);

    printf_("%s", "INFO: realptr: ");
    printf_("0x%llx\n", realPtr);

    frame_free_multiple(realPtr, pages);

    return 0;
}