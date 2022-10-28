#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include "include/string.h"
#include "include/liballoc.h"
#include "include/shell.h"
#include "include/printf.h"
#include "include/drivers/keyboard/keyboard.h"
#include "include/time.h"

void vsh_loop()
{

    char *line;

    int status;

    do
    {

        printf_("%s", ":> ");

        line = vsh_readline();

        status = 1;

        cmd_parser(line);

        free(line);

        status = 0;

    } while (status);
}

char *vsh_readline()
{

    int bufsize = VSH_CMD_BUFFER_SIZE;
    int pos = 0;
    char *buffer = malloc(sizeof(char) * bufsize);
    int c;

    if (!buffer)
    {

        printf_("%s\n", "vsh: Command Buffer Allocation Error!");
        // return;
    }

    while (1)
    {

        c = k_getchar();

     

        if (c == '+')
        {

            return buffer;
        }
        else
        {

            buffer[pos] = c;
        }

        pos++;

        if (pos >= bufsize)
        {

            bufsize += VSH_CMD_BUFFER_SIZE;
            buffer = realloc(buffer, bufsize);

            if (!buffer)
            {

                printf_("%s\n", "vsh: Command Buffer Allocation Error!");
                // return;
            }
        }
    }
}

void cmd_parser(const char str)
{

    if (str == "time")
    {

        print_sys_time();
    }
    else
    {

        printf_("%s\n", "unknown command");
    }
}