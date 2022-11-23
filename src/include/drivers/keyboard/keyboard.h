#pragma once

#define KEYBOARD_DATA_PORT 0x60
#define KEYBOARD_STATUS_PORT 0x64

void keyboard_handler();

void keyboard_init();

char k_getchar();

char kbd_pop();

void keyboard_handler_2();

void read_input();