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
    int id = current->pid;

    for (;;)
    {
        printf_("Soaker PID %i Alive\n", id);
        schedule();
    }
}