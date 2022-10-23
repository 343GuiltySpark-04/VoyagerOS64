#include "../../include/drivers/keyboard/keyboard_map.h"
#include "../../include/drivers/keyboard/keyboard.h"
#include "../../include/io.h"
#include "../../include/pic.h"
#include "../../include/printf.h"

void keyboard_handler()
{

    uint8_t status = inb(KEYBOARD_STATUS_PORT);

    if (status & 0x1)
    {

        char keycode = inb(KEYBOARD_DATA_PORT);

        if (keycode < 0 || keycode >= 128)
        {
            return;
        }

        printf_("%c\n", keyboard_map[keycode]);
        return;
    }
}

void keyboard_init()
{
    pic_unmask_irq(1);
    printf_("%s\n", "Keyboard Init");
}
