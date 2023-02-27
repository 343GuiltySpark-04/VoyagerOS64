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

    // Set the maximum number of frames to be returned.
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

void stack_dump()
{

    struct stack_frame *stk;
    asm volatile("mov %%rbp,%0"
                 : "=r"(stk)::);

    dump_hex(stk, 0x1000);
}

void stack_dump_recursive(uint64_t max_frames)
{

    // Set the maximum number of frames to be returned.
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

        dump_hex(stk->rbp, 0x1000);
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

/**
 * @brief Dumps a block of data as hex. This is useful for debugging and to ensure that the data is in hex format. credits to: https://gist.github.com/ccbrown/9722406
 * @param data Pointer to the data to dump. Must be aligned to 8 bytes.
 * @param size Size of the data in bytes. Must be aligned to 8 bytes
 */
void dump_hex(const void *data, size_t size)
{
    char ascii[17];
    size_t i, j;
    ascii[16] = '\0';
    // Prints the ascii representation of the data.
    for (i = 0; i < size; ++i)
    {
        printf("%02X ", ((unsigned char *)data)[i]);
        // Convert a character to ascii character
        if (((unsigned char *)data)[i] >= ' ' && ((unsigned char *)data)[i] <= '~')
        {
            ascii[i % 16] = ((unsigned char *)data)[i];
        }
        else
        {
            ascii[i % 16] = '.';
        }
        // Prints the ascii string at the current position.
        if ((i + 1) % 8 == 0 || i + 1 == size)
        {
            printf(" ");
            // Prints the ascii string at position i.
            if ((i + 1) % 16 == 0)
            {
                printf("|  %s \n", ascii);
            }
            // Prints the ascii string at position i.
            else if (i + 1 == size)
            {
                ascii[(i + 1) % 16] = '\0';
                // Prints a space between 0 and 8
                if ((i + 1) % 16 <= 8)
                {
                    printf(" ");
                }
                // Prints the number of characters in the range 0 16.
                for (j = (i + 1) % 16; j < 16; ++j)
                {
                    printf("   ");
                }
                printf("|  %s \n", ascii);
            }
        }
    }
}