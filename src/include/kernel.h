#pragma once
#include <stdint.h>

#ifndef _KERNEL_H
#define _KENREL_H

extern struct standby_tube k_standby_tube;

extern struct active_tube k_active_tube;

extern struct process_list k_process_list;

extern uint8_t init_done;

extern uint32_t bootspace;

extern uint8_t kerror_mode;

extern volatile struct limine_terminal_request early_term;

#endif