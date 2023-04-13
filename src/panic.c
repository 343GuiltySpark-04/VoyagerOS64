#include "include/printf.h"
#include <stdint.h>
#include <stdbool.h>
#include <stdarg.h>

extern void halt();

/**
 * @brief Prints a message and halts the program. This is useful for errors that are expected to cause the program to exit and not be able to recover from a fatal error.
 * @param fmt A printf - style format string to print to
 */
void panic(const char *fmt, ...)
{

    static char buf[1024];

    va_list args;

    va_start(args, fmt);

    vsnprintf(buf, sizeof(buf), fmt, args);
    va_end(args);
    printf_("%s\n", "!!!!!!!!!!!!!!!!!!!!KERNEL PANIC!!!!!!!!!!!!!!!!!!!!!");
    printf("%s\n", buf);
    printf_("%s\n", "!!!!!!!!!!!!!!!!!!!!KERNEL PANIC!!!!!!!!!!!!!!!!!!!!!");
    halt();
}