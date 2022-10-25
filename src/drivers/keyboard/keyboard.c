#include "../../include/drivers/keyboard/keyboard_map.h"
#include "../../include/drivers/keyboard/keyboard.h"
#include "../../include/io.h"
#include "../../include/pic.h"
#include "../../include/printf.h"
#include "../../include/KernelUtils.h"
#include "../../include/terminal/term.h"
#include "../../include/liballoc.h"

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

       // const char character = keyboard_map[keycode];

        printf_("%c\n", keyboard_map[keycode]);

     term_write(term_context, keyboard_map[keycode], sizeof(char));

        return;
    }
}

void keyboard_init()
{
    pic_unmask_irq(1);
    printf_("%s\n", "Keyboard Init");
}
