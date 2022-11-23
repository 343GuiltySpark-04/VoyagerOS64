#pragma once
#include <stdint.h>

extern uint32_t bootspace;

extern uint8_t kerror_mode;

extern volatile struct limine_terminal_request early_term;

void clear_screen();

void print_prompt();