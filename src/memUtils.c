#include "include/paging/frameallocator.h"
#include "include/paging/paging.h"

/**
* @brief This function returns the number of free frames.
* @return The number of free frames
*/
PAGING_EXPORT uint64_t PagingGetFreeFrame()
{

    return (uint64_t)frame_request();
}