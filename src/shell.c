#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include "include/string.h"
#include "include/liballoc.h"
#include "include/shell.h"
#include "include/printf.h"
#include "include/drivers/keyboard/keyboard.h"
#include "include/time.h"
#include "include/lock.h"

/**
 * @brief This is the vsh loop.
 * @return 0 on success non - zero
 */
void vsh_loop()
{

    char *line;

    int status;

    printf_("%s", ":> ");

    line = vsh_readline();

    cmd_parser(line);

    free(line);
}

/**
 * @brief Read a line from VSH and return it.
 * @return char The line that was read
 */
char vsh_readline()
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

    L1:

        if (!buffer)
        {

            printf_("%s\n", "vsh: Command Buffer Allocation Error!");
            return; // Added return statement here
        }

        while (1)
        {

            while ((c = kbd_pop()) == 0)
                ; // Replaced goto statement with while loop

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
                    return; // Added return statement here
                }
            }
        }
    }
}

/**
 * @brief Parses and prints command.
 * @param char
 */
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