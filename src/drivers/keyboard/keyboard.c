#include "../../include/drivers/keyboard/keyboard_map.h"
#include "../../include/drivers/keyboard/keyboard.h"
#include "../../include/io.h"
#include "../../include/pic.h"
#include "../../include/printf.h"
#include "../../include/KernelUtils.h"
#include "../../include/terminal/term.h"
#include "../../include/liballoc.h"
#include "../../include/serial.h"
#include "../../include/lock.h"
#include "../../include/kernel.h"
#include "../../include/cpuUtils.h"

#define KBD_STACK_SIZE 255

char k_char;

char kbd_stack[KBD_STACK_SIZE];

int kbd_top = -1;

static spinlock_t reader_lock = SPINLOCK_INIT;

void kbd_push(char data)
{

    if (kbd_top == KBD_STACK_SIZE - 1)
    {

        kerror_mode = 1;

        printf_("%s\n", "ERROR: Keyboard stack overflow!");

        kerror_mode = 0;
    }
    else
    {

        kbd_top++;
        kbd_stack[kbd_top] = data;
    }
}

char kbd_pop()
{

    int data;

    if (kbd_top == -1)
    {

        return 0;
    }
    else
    {

        data = kbd_stack[kbd_top];

        kbd_top--;

        return data;
    }
}

void keyboard_handler()
{

    uint8_t status = inb(KEYBOARD_STATUS_PORT);

    //  printf_("0x%llx\n", status);

    if (status & 0x1)
    {

        char keycode = inb(KEYBOARD_DATA_PORT);

        inb(KEYBOARD_DATA_PORT);

        kbd_push(keyboard_map[keycode]);

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

void keyboard_handler_2()
{

    uint8_t status = inb(KEYBOARD_STATUS_PORT);

    if (status & 0x1)
    {
        char keycode = inb(KEYBOARD_DATA_PORT);
        inb(KEYBOARD_DATA_PORT);

        if (keycode < 0 || keycode >= 128)
        {
            return;
        }

        kbd_push(keyboard_map[keycode]);

        if (keycode == 28)
        {

            kbd_pop();

            kbd_push(28);
        }

        clear_screen();

        // printf_("%i\n", keycode);

        read_input();

        // printf_("%s\n", "Test");
    }
}

void read_input()
{

    spinlock_acquire(&reader_lock);

    char option = kbd_pop();

    if (option == 28)
    {
        clear_screen();
        print_prompt();
    }

    switch (option)
    {

    case '1':
        print_memmap();
        printf_("%s\n", "Press Enter to Return to The Menu.");
        break;

    case '2':
        printf_("%s\n", "I suggest you check the results with the Intel and AMD dev manuals.");
        cpuid_readout();
        printf_("%s\n", "Press Enter to Return to The Menu.");
        break;

    case '3':
        print_memory();
        printf_("%s\n", "Press Enter to Return to The Menu.");
        break;

    case '4':
        system_readout();
        printf_("%s\n", "Press Enter to Return to The Menu.");
        break;

    default:
        clear_screen();
        print_prompt();
    }

    spinlock_release(&reader_lock);

    // printf_("%s\n", &option);
}

char k_getchar()
{

    char c;

    while (k_char <= 0)
    {

        c = k_char;
        k_char = 0;
    }

    return c;
}

void keyboard_init()
{

    pic_unmask_irq(1);
    inb(KEYBOARD_DATA_PORT);
    inb(KEYBOARD_DATA_PORT);

    printf_("%s\n", "Keyboard Init");
}
