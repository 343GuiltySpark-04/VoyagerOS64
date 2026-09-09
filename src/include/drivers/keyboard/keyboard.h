#pragma once

#define KEYBOARD_DATA_PORT 0x60
#define KEYBOARD_STATUS_PORT 0x64

void keyboard_handler(void);
void keyboard_init(void);
char k_getchar(void);
char kbd_pop(void);
