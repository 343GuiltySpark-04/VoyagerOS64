#include <stdint.h>

#pragma once
#ifndef SERIAL_H
#define SERIAL_H

extern void     serial_debug(char c);
extern uint64_t get_ts();

void inline serial_print(const char *str);
void inline serial_print_line(const char *str);

#endif
