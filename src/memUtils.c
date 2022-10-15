#include "include/paging/frameallocator.h"
#include "include/paging/paging.h"

PAGING_EXPORT uint64_t PagingGetFreeFrame()
{

    return (uint64_t)frame_request();
}