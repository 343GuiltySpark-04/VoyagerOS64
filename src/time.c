#include "include/io.h"
#include "include/printf.h"
#include "include/pic.h"
#include <stdint.h>

#define CHANNEL_ZERO 0x40
#define MODE_CMD 0x43

extern void config_PIT(uint8_t freq);

uint64_t system_timer_ms = 0;

uint64_t system_timer_fractions = 0;

void sys_clock_handler()
{

    system_timer_fractions++;
    system_timer_ms++;
}

void init_PIT()
{

    config_PIT(1000);

    pic_unmask_irq(0);

    printf_("%s\n", "PIT Online!");
}