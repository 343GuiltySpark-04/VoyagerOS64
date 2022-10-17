#include "include/term.h"
#include "include/vgafont.h"
#include "include/printf.h"
#include "include/string.h"
#include "include/kernel.h"
#include "include/liballoc.h"

void *alloc_mem(size_t size)
{

    malloc((uint64_t)size);
    // memset(); zero memory?
}
void free_mem(void *ptr, size_t size)
{

    free(ptr); // i feel like theres more??
}
