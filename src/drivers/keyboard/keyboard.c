#include "../../include/drivers/keyboard/keyboard_map.h"
#include "../../include/drivers/keyboard/keyboard.h"
#include "../../include/io.h"
#include "../../include/pic.h"
#include "../../include/printf.h"
#include "../../include/KernelUtils.h"
#include "../../include/terminal/term.h"
#include "../../include/liballoc.h"
#include "../../include/serial.h"

char k_char;

void keyboard_handler()
{

    uint8_t status = inb(KEYBOARD_STATUS_PORT);

    //  printf_("0x%llx\n", status);

    if (status & 0x1)
    {

        char keycode = inb(KEYBOARD_DATA_PORT);

        inb(KEYBOARD_DATA_PORT);

        if (keycode < 0 || keycode >= 128)
        {
            return;
        }

        // printf_("0x%llx\n", keyboard_map[keycode]);
        // printf_("0x%llx\n", keycode);

        if (keycode == 0x1c)
        {

            k_char = '+';
        }
        else
        {

            k_char = keyboard_map[keycode];
        }

        //  printf_("%s\n", &k_char);

        if (term_context)
        {

           // term_write(term_context, &keyboard_map[keycode], sizeof(char));
        }
        else
        {

            printf_("%s\n", "ERROR: TERMINAL WRITE FAILURE!");

            return;
        }

        return;
    }
}

char k_getchar()
{

    char c;

    while (k_char <= 0)
    {

        c = k_char;
        k_char = 0;

        
        return c;
    }
}

void keyboard_init()
{
    pic_unmask_irq(1);
    printf_("%s\n", "Keyboard Init");
}
