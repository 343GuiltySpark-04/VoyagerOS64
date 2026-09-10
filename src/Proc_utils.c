/**
 * Copyright (c) 2026 Tristan Adams
 *
 * This software is released under the MIT License.
 * https://opensource.org/licenses/MIT
 */

#include "include/printf.h"
#include "include/sched.h"

uint8_t soak_counter = 0;

void soak(void)
{
    soak_counter++;

    for (;;)
    {
        printf_("%s\n", "Soaker " + soak_counter, " Alive");
        schedule();
    }
}