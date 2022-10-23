#include "include/printf.h"
#include "include/serial.h"
#include "include/kernel.h"
#include "include/term.h"
#include "include/termUtils.h"

void _putchar(char character)
{

    /// @brief lets the printf function use the serial or terminal
    /// depending if the kernel has setup memory for the terminal or not.
    /// @param character
    /*  if (bootspace == 1)
     {

         serial_debug(character);
     }
     else
     {
         // Enable when needed
         // serial_debug(character);
         term_putchar(&term, character);

     } */

    serial_debug(character);
}
