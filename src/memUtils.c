#include "include/paging/frameallocator.h"
#include "include/paging/paging.h"
#include "include/pebble.h"
#include "include/memUtils.h"
#include <stddef.h>

/**
 * @brief This function returns the number of free frames.
 * @return The number of free frames
 */
PAGING_EXPORT uint64_t PagingGetFreeFrame()
{

    return (uint64_t)frame_request();
}

// just a wrapper to make useage easier and code more readable
/**
 * @brief Allocate memory in virtual memory. This is equivalent to pmalloc ( size, alingment, MALLOC_FLAGS_VIRTUAL )
 * @param size number of bytes to allocate
 */
void vmalloc(size_t size)
{

    return pmalloc(size, 0, MALLOC_FLAGS_VIRTUAL);
}