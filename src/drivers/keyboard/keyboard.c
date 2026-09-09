#include "../../include/drivers/keyboard/keyboard.h"
#include "../../include/drivers/keyboard/keyboard_map.h"
#include "../../include/io.h"
#include "../../include/kernel.h"
#include "../../include/pic.h"
#include "../../include/printf.h"
#include "../../include/terminal/term.h"
#include <stdint.h>

#define KBD_STACK_SIZE 255

char k_char;
char kbd_stack[KBD_STACK_SIZE];

static volatile uint16_t kbd_head = 0;
static volatile uint16_t kbd_tail = 0;

/**
 * @brief Push a character onto the keyboard FIFO.
 * @param data Character to enqueue.
 */
void kbd_push(char data)
{
    uint16_t next = (uint16_t)((kbd_head + 1u) % KBD_STACK_SIZE);

    if (next == kbd_tail)
    {
        kerror_mode = 1;
        printf_("%s\n", "ERROR: Keyboard buffer overflow!");
        kerror_mode = 0;
        return;
    }

    kbd_stack[kbd_head] = data;
    kbd_head = next;
}

/**
 * @brief Pop the oldest character from the keyboard FIFO.
 * @return Queued character, or 0 when the queue is empty.
 */
char kbd_pop(void)
{
    if (kbd_tail == kbd_head)
        return 0;

    char data = kbd_stack[kbd_tail];
    kbd_tail = (uint16_t)((kbd_tail + 1u) % KBD_STACK_SIZE);
    return data;
}

/**
 * @brief PS/2 keyboard IRQ handler.
 *
 * Key-release scancodes are ignored. Enter is normalized to a newline for the
 * shell while k_char retains the historical '+' sentinel for legacy callers.
 */
void keyboard_handler(void)
{
    uint8_t status = inb(KEYBOARD_STATUS_PORT);
    if ((status & 0x1u) == 0)
        return;

    uint8_t keycode = inb(KEYBOARD_DATA_PORT);

    /* Set 1 break codes have the high bit set. */
    if (keycode & 0x80u)
        return;

    if (keycode >= 128u)
        return;

    if (keycode == 0x1cu)
    {
        k_char = '+';
        kbd_push('\n');
        return;
    }

    char mapped = keyboard_map[keycode];
    k_char = mapped;

    if (mapped != 0)
        kbd_push(mapped);
}

/**
 * @brief Fetch the most recently translated keyboard character.
 * @return Character, or 0 if none is pending.
 */
char k_getchar(void)
{
    char c = k_char;
    k_char = 0;
    return c;
}

/**
 * @brief Enable keyboard IRQ1.
 */
void keyboard_init(void)
{
    pic_unmask_irq(1);
    printf_("%s\n", "Keyboard Init");
}
