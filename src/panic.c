#include "include/printf.h"
#include <stdint.h>
#include <stdbool.h>
#include <stdarg.h>
#include "include/stack_trace.h"
#include "include/KernelUtils.h"
#include "include/kernel.h"

extern void halt();

/**
 * @brief Panic with a nicely formatted message. This is useful for situations where you are certain an unrecoverable error will occur and want to be informative (feel free to abuse sarcasm in them) about the problem.
 * @param fmt printf style format string for the panic message
 */
void panic(const char *fmt, ...)
{

    if (init_done == 1)
    {

        kerror_mode = 1;
    }
    static char buf[1024];

    va_list args;

    va_start(args, fmt);

    vsnprintf(buf, sizeof(buf), fmt, args);
    va_end(args);
    printf_("%s\n", "!!!!!!!!!!!!!!!!!!!!KERNEL PANIC!!!!!!!!!!!!!!!!!!!!!");
    printf_("%s\n", "-----------------------------------------------------");
    printf("%s", "CAUSE: ");
    printf("%s\n", buf);

    printf_("%s\n", "-----------------------------------------------------");

    stack_trace(0);
    printf("%s\n", "-----------------------------------------------");
    printf("%s\n", "    INFO: Stack Trace (Dump) as Follows");
    printf("%s\n", "-----------------------------------------------");
    stack_dump_asm(0, 256);
    printf("%s\n", "-----------------------------------------------");
    printf_("%s\n", "End of Trace.");
    printf("%s\n", "-----------------------------------------------");

    printf_("%s\n", "!!!!!!!!!!!!!!!!!!!!KERNEL PANIC!!!!!!!!!!!!!!!!!!!!!");

    halt();
}