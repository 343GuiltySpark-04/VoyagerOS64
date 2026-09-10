#include "include/stack_trace.h"
#include "include/KernelUtils.h"
#include "include/global_defs.h"
#include "include/limine.h"
#include "include/printf.h"
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

extern volatile struct limine_stack_size_request stack_req;
extern uint64_t walk_stack(uint64_t *array, uint64_t max);
extern void     halt(void);

void stack_trace_asm(uint64_t max_size, bool stop)
{
    uint64_t addresses[max_size];
    uint64_t array_max = walk_stack(addresses, max_size);

    printf("%s\n", "--------------------------------------");
    printf("%s\n", "    INFO: Stack Trace as Follows");
    printf("%s\n", "--------------------------------------");
    printf("%s", "Size of address list: ");
    printf("%llx\n", array_max);

    if (array_max > 0)
    {
        for (uint64_t i = 0; i + 1 < array_max; i++)
            printf("0x%llx\n", addresses[i]);
    }

    if (stop)
        halt();
}

void stack_trace(uint64_t max_frames)
{
    if (max_frames == 0)
        max_frames = k_mode.stack_trace_size;

    struct stack_frame *stk;
    asm volatile("mov %%rbp,%0" : "=r"(stk)::);
    printf("%s\n", "-----------------------------------------------");
    printf("%s\n", "    INFO: Stack Trace (Unwind) as Follows");
    printf("%s\n", "-----------------------------------------------");

    for (uint8_t frame = 0; stk && frame < max_frames; ++frame)
    {
        printf_("0x%llx\n", stk->rip);
        stk = stk->rbp;
    }
    printf("%s\n", "-----------------------------------------------");
    printf_("%s\n", "End of Trace.");
    printf("%s\n", "-----------------------------------------------");
}

void stack_dump(void)
{
    struct stack_frame *stk;
    asm volatile("mov %%rbp,%0" : "=r"(stk)::);
    dump_hex(stk, 0x1000);
}

void stack_dump_recursive(uint64_t max_frames)
{
    if (max_frames == 0)
        max_frames = k_mode.stack_trace_size;

    struct stack_frame *stk;
    asm volatile("mov %%rbp,%0" : "=r"(stk)::);
    printf("%s\n", "-----------------------------------------------");
    printf("%s\n", "    INFO: Stack Trace (Unwind) as Follows");
    printf("%s\n", "-----------------------------------------------");

    for (uint8_t frame = 0; stk && frame < max_frames; ++frame)
    {
        dump_hex(stk->rbp, 0x1000);
        stk = stk->rbp;
    }
    printf("%s\n", "-----------------------------------------------");
    printf_("%s\n", "End of Trace.");
    printf("%s\n", "-----------------------------------------------");
}

void print_stack_size(void)
{
    if (stack_req.stack_size == 0)
    {
        printf_("%s\n", "ERROR: Unable to fetch stack size!");
    }
    else
    {
        printf_("%s", "INFO: Stack Size: ");
        printf_("0x%llx\n", stack_req.stack_size);
    }
}

void dump_hex(const void *data, size_t size)
{
    char   ascii[17];
    size_t i, j;
    ascii[16] = '\0';

    for (i = 0; i < size; ++i)
    {
        printf("%02X ", ((unsigned char *) data)[i]);
        if (((unsigned char *) data)[i] >= ' ' &&
            ((unsigned char *) data)[i] <= '~')
        {
            ascii[i % 16] = ((unsigned char *) data)[i];
        }
        else
        {
            ascii[i % 16] = '.';
        }

        if ((i + 1) % 8 == 0 || i + 1 == size)
        {
            printf(" ");
            if ((i + 1) % 16 == 0)
            {
                printf("|  %s \n", ascii);
            }
            else if (i + 1 == size)
            {
                ascii[(i + 1) % 16] = '\0';
                if ((i + 1) % 16 <= 8)
                    printf(" ");
                for (j = (i + 1) % 16; j < 16; ++j)
                    printf("   ");
                printf("|  %s \n", ascii);
            }
        }
    }
}
