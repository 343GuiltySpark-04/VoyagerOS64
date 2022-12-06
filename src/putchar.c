#include "include/printf.h"
#include "include/serial.h"
#include "include/kernel.h"
#include "include/terminal/term.h"
#include "include/KernelUtils.h"

void _putchar(char character)
{

    /// @brief lets the printf function use the serial or terminal
    /// depending if the kernel has setup memory for the terminal or not.
    /// @param character
    if (kerror_mode == 1)
    {

        term_write(term_context, &character, sizeof(char));
        serial_debug(character);
    }
    else if (kerror_mode == 2)
    {

        serial_debug(character);
    }
    else if (bootspace == 1)
    {

        serial_debug(character);
    }
    else if (bootspace == 2)
    {

        early_term.response->write(early_term.response->terminals[0], &character, sizeof(char));
        serial_debug(character);
    }

    else if (bootspace == 3)
    {

        early_term.response->write(early_term.response->terminals[0], &character, sizeof(char));
    }

    else
    {
        if (term_context)
        {

            term_write(term_context, &character, sizeof(char));
        }
        else
        {

            serial_debug(character);
            printf_("%s\n", "ERROR: TERMINAL WRITE FAILURE!");
            printf_("%s\n", "Defaulting to serial");

            return;
        }
    }

   
}
