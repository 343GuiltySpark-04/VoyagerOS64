#include "include/printf.h"
#include <stdint.h>
#include <stddef.h>
#include "include/stack_trace.h"
#include "include/global_defs.h"
#include <stdbool.h>
#include "include/limine.h"
#include "include/KernelUtils.h"

volatile struct limine_stack_size_request stack_req = {

    .id = LIMINE_STACK_SIZE_REQUEST,
    .revision = 0,
    .stack_size = 4096

};

extern uint64_t walk_stack(uint64_t *array, uint64_t max);
extern void halt();

/**
 * @brief Walk the stack and print it to stdout. This is a debugging function. It will print the address list as follows : 1. Size of the stack trace is equal to max_size. 2. Address list is walked from bottom to top and all addresses below it are printed.
 * @param max_size The maximum size of the stack trace.
 * @param stop If true the function will halt after printing the stack trace
 */
void stack_trace_asm(uint64_t max_size, bool stop)
{

    uint64_t addresses[max_size];
    uint64_t array_max;

    array_max = walk_stack(addresses, max_size);

    printf("%s\n", "--------------------------------------");
    printf("%s\n", "    INFO: Stack Trace as Follows");
    printf("%s\n", "--------------------------------------");
    printf("%s", "Size of address list: ");
    printf("%llx\n", array_max);

    // Prints the addresses in the array.
    for (uint64_t i = 0; i < array_max - 1; i++)
    {

        printf("0x%llx\n", addresses[i]);
    }

    // Stop the process if stop is true.
    if (stop == true)
    {

        halt();
    }
}

/**
 * @brief Print a stack trace. Unwind to the previous frame is printed as well as the stack trace of the function being called.
 * @param max_frames The maximum number of frames to unwind. To use the maximum value set in the k_mode struct set this to 0 or NULL.
 */
void stack_trace(uint64_t max_frames)
{

    if (max_frames == 0 || NULL)
    {

        max_frames = k_mode.stack_trace_size;
    }

    struct stack_frame *stk;
    asm volatile("mov %%rbp,%0"
                 : "=r"(stk)::);
    printf("%s\n", "-----------------------------------------------");
    printf("%s\n", "    INFO: Stack Trace (Unwind) as Follows");
    printf("%s\n", "-----------------------------------------------");

    // Prints out the stack frame.
    for (uint8_t frame = 0; stk && frame < max_frames; ++frame)
    {

        printf_("0x%llx\n", stk->rip);
        stk = stk->rbp;
    }
    printf("%s\n", "-----------------------------------------------");
    printf_("%s\n", "End of Trace.");
    printf("%s\n", "-----------------------------------------------");
}

/**
 * @brief \ brief Print the stack size to stdout. \ param [ in ] none
 */
void print_stack_size()
{

    // Prints the stack size of the request.
    if (stack_req.stack_size == NULL)
    {

        printf_("%s\n", "ERROR: Unable to fetch stack size!");
    }
    else
    {

        printf_("%s", "INFO: Stack Size: ");
        printf_("0x%llx\n", stack_req.stack_size);
    }
}
