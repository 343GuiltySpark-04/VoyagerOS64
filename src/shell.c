#include "include/drivers/keyboard/keyboard.h"
#include "include/mm/kmalloc.h"
#include "include/mm/memtest.h"
#include "include/printf.h"
#include "include/proc_utils.h"
#include "include/sched.h"
#include "include/shell.h"
#include "include/time.h"
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

extern void halt(void);

static bool shell_streq(const char *a, const char *b)
{
    if (!a || !b)
        return false;

    while (*a && *b)
    {
        if (*a != *b)
            return false;
        ++a;
        ++b;
    }

    return *a == '\0' && *b == '\0';
}

/* Public VSH API documentation lives in include/shell.h. */
void vsh_loop(void)
{
    for (;;)
    {
        printf_("%s", ":> ");

        char *line = vsh_readline();
        if (line)
        {
            cmd_parser(line);
            kfree(line);
        }

        schedule();
    }
}

char *vsh_readline(void)
{
    size_t bufsize = VSH_CMD_BUFFER_SIZE;
    size_t pos     = 0;
    char  *buffer  = kmalloc(bufsize);

    if (!buffer)
    {
        printf_("%s\n", "vsh: Command Buffer Allocation Error!");
        return NULL;
    }

    for (;;)
    {
        char c = kbd_pop();

        if (c == 0)
        {
            schedule();
            continue;
        }

        if (c == '\n' || c == '\r')
        {
            buffer[pos] = '\0';
            printf_("%s\n", "");
            return buffer;
        }

        if (c == '\b')
        {
            if (pos > 0)
            {
                --pos;
                printf_("%s", "\b \b");
            }
            continue;
        }

        if (pos + 1 >= bufsize)
        {
            size_t newsize = bufsize + VSH_CMD_BUFFER_SIZE;
            char  *grown   = krealloc(buffer, newsize);
            if (!grown)
            {
                printf_("%s\n", "vsh: Command Buffer Allocation Error!");
                kfree(buffer);
                return NULL;
            }

            buffer  = grown;
            bufsize = newsize;
        }

        buffer[pos++] = c;

        char echo[2] = {c, '\0'};
        printf_("%s", echo);
    }
}

void cmd_parser(const char *str)
{
    if (!str || str[0] == '\0')
        return;

    if (shell_streq(str, "time"))
    {
        print_sys_time();
        return;
    }

    if (shell_streq(str, "memtest"))
    {
        memtest_run();
        return;
    }

    if (shell_streq(str, "halt") || shell_streq(str, "Halt"))
    {
        printf_("%s", "Halting System...");
        halt();
    }

    if (shell_streq(str, "soak"))
    {
        create_process(soak);
        return;
    }

    else
    {
        printf_("%s\n", "unknown command");
    }
}
