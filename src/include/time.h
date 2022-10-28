#pragma once


#ifndef _TIME_H
#define _TIME_H

extern uint64_t system_timer_ms;

extern uint64_t system_timer_fractions;

void sys_clock_handler();

void init_PIT();

#endif