#ifndef SERIAL_H
#define SERIAL_H

extern void serial_debug(int c);

void inline serial_print(const char *str);
void inline serial_print_line(const char *str);

#endif
