#include "include/term.h"
#include "include/vgafont.h"
#include "include/printf.h"
#include "include/string.h"
#include "include/kernel.h"
#include "include/liballoc.h"
#include "include/KernelUtils.h"
#include <stddef.h>

void *alloc_mem(size_t size)
{

    return (malloc((uint64_t)size));
}
void free_mem(void *ptr, size_t size)
{

    free(ptr); // i feel like theres more??
}

