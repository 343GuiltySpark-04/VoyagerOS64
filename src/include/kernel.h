#pragma once
#ifndef _KERNEL_H
#define _KERNEL_H

#include "limine.h"
#include <stdint.h>

extern uint32_t bootspace;
extern uint8_t  kerror_mode;
extern uint8_t  init_done;

extern volatile struct limine_terminal_request early_term;

#endif
