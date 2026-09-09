#include <stdint.h>

#pragma once
#ifndef SERIAL_H
#define SERIAL_H

extern void     serial_debug(char c);
extern uint64_t get_ts(void);

void serial_print(const char *str);
void serial_print_line(const char *str);
void serial_print_with_ts(const char *msg);

#endif
